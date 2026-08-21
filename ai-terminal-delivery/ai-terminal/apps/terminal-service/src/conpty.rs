//! Windows ConPTY bindings.
//!
//! This module is deliberately isolated from UI and parser code. The production runner owns
//! the input/output pipe handles, drains output on a dedicated reader thread, and calls close
//! from a non-reader thread. Microsoft documents that old Windows versions can block if output
//! is not closed or drained before ClosePseudoConsole.

use crate::model::GridSize;

#[cfg(windows)]
mod implementation {
    use super::GridSize;
    use std::ffi::c_void;
    use std::io;
    use std::mem::MaybeUninit;
    use std::os::windows::io::{AsRawHandle, RawHandle};

    type Handle = isize;
    type Hpc = isize;
    type HResult = i32;

    #[repr(C)]
    #[derive(Clone, Copy)]
    struct Coord {
        x: i16,
        y: i16,
    }

    #[link(name = "kernel32")]
    extern "system" {
        fn CreatePseudoConsole(
            size: Coord,
            input: Handle,
            output: Handle,
            flags: u32,
            pseudoconsole: *mut Hpc,
        ) -> HResult;
        fn ResizePseudoConsole(pseudoconsole: Hpc, size: Coord) -> HResult;
        fn ClosePseudoConsole(pseudoconsole: Hpc);
    }

    fn coord(grid: GridSize) -> io::Result<Coord> {
        let cols = i16::try_from(grid.cols).map_err(|_| {
            io::Error::new(io::ErrorKind::InvalidInput, "columns exceed ConPTY COORD")
        })?;
        let rows = i16::try_from(grid.rows)
            .map_err(|_| io::Error::new(io::ErrorKind::InvalidInput, "rows exceed ConPTY COORD"))?;
        Ok(Coord { x: cols, y: rows })
    }

    /// Owns only the HPCON handle. Pipe lifetime is owned by the caller's session worker.
    pub struct PseudoConsole {
        handle: Hpc,
    }

    impl PseudoConsole {
        /// `input` is terminal-to-child; `output` is child-to-terminal. Both handles must use
        /// synchronous I/O per the ConPTY contract.
        pub unsafe fn create(
            input: RawHandle,
            output: RawHandle,
            grid: GridSize,
        ) -> io::Result<Self> {
            let mut handle = MaybeUninit::<Hpc>::uninit();
            let hr = CreatePseudoConsole(
                coord(grid)?,
                input as Handle,
                output as Handle,
                0,
                handle.as_mut_ptr(),
            );
            if hr < 0 {
                return Err(io::Error::from_raw_os_error(hr));
            }
            Ok(Self {
                handle: handle.assume_init(),
            })
        }

        pub fn resize(&self, grid: GridSize) -> io::Result<()> {
            let hr = unsafe { ResizePseudoConsole(self.handle, coord(grid)?) };
            if hr < 0 {
                return Err(io::Error::from_raw_os_error(hr));
            }
            Ok(())
        }
    }

    impl Drop for PseudoConsole {
        fn drop(&mut self) {
            // Caller must already have closed input and arranged output draining. Drop is a last
            // resort, not the normal session-close path.
            unsafe { ClosePseudoConsole(self.handle) };
        }
    }

    pub fn is_supported() -> bool {
        true
    }
    pub use PseudoConsole as PublicPseudoConsole;
}

#[cfg(windows)]
pub use implementation::PublicPseudoConsole as PseudoConsole;

#[cfg(windows)]
pub fn is_supported() -> bool {
    implementation::is_supported()
}

#[cfg(not(windows))]
pub struct PseudoConsole;

#[cfg(not(windows))]
impl PseudoConsole {
    pub fn is_supported() -> bool {
        false
    }
}

#[cfg(not(windows))]
pub fn is_supported() -> bool {
    false
}
