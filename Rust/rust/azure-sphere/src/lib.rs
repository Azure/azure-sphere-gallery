pub mod applibs;

#[cfg(not(feature = "std"))]
pub extern crate alloc;
#[cfg(feature = "std")]
extern crate std;
#[cfg(feature = "std")]
pub extern crate std as alloc;
