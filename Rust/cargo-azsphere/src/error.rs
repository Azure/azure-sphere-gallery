use cargo_toml::Error as CargoTomlError;
use std::error::Error as StdError;
use std::fmt::{Debug, Display, Formatter};
use std::io::Error as IoError;
use std::path::PathBuf;
use toml::de::Error as TomlDeError;

#[derive(thiserror::Error, Debug, Clone, PartialEq, Eq, Hash)]
pub enum DottedBareKeyLexError {
    #[error("invalid key-joint character `.'")]
    InvalidDotChar,
    #[error("invalid character `{0}' and quoted key is not supported")]
    QuotedKey(char),
    #[error("invalid character `{0}'")]
    InvalidChar(char),
}

#[derive(thiserror::Error, Debug, Clone)]
pub enum ConfigError {
    #[error("Missing field: {0}")]
    Missing(String),
    #[error("Field {0} must be {1}")]
    WrongType(String, &'static str),
    #[error("Invalid branch path `{0}'")]
    WrongBranchPathOfToml(String, #[source] DottedBareKeyLexError),
    #[error("Branch `{0}' not found")]
    BranchPathNotFoundInToml(String),
    #[error("Expected one string or two in array")]
    MalFormedArray,
}

#[derive(thiserror::Error, Debug)]
pub struct FileAnnotatedError<E: StdError + Display>(pub Option<PathBuf>, #[source] pub E);

impl<E: StdError + Display> Display for FileAnnotatedError<E> {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match &self.0 {
            None => Display::fmt(&self.1, f),
            Some(path) => write!(f, "{}: {}", path.as_path().display(), self.1),
        }
    }
}

#[derive(thiserror::Error, Debug)]
pub enum Error {
    #[error(transparent)]
    Config(#[from] ConfigError),

    #[error(transparent)]
    ExtraConfig(#[from] FileAnnotatedError<ConfigError>),

    #[error("Cargo.toml: {0}")]
    CargoToml(#[from] CargoTomlError),

    #[error("{1}: {0}")]
    FileIo(PathBuf, #[source] IoError),

    #[error(transparent)]
    Io(#[from] IoError),

    #[error(transparent)]
    ParseTomlFile(#[from] FileAnnotatedError<TomlDeError>),
}
