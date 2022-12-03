use crate::config::ExtraMetadataSource;
use crate::error::{ConfigError, FileAnnotatedError};
use crate::Error;
use cargo_toml::Manifest;
use std::fs;
use std::path::PathBuf;
use toml::value::Table;
use toml::Value;

mod toml_dotted_bare_key_parser {
    use crate::error::DottedBareKeyLexError;

    pub(super) fn parse_dotted_bare_keys(input: &str) -> Result<Vec<&str>, DottedBareKeyLexError> {
        let mut keys = Vec::new();

        let mut pos = 0;
        while pos < input.len() {
            if let Some(key_end) = input
                .bytes()
                .enumerate()
                .skip(pos)
                .take_while(|(_, b)| {
                    // a bare key may contain a-zA-Z0-9_-
                    b.is_ascii_alphanumeric() || *b == b'_' || *b == b'-'
                })
                .last()
                .map(|(i, _)| i)
            {
                keys.push(&input[pos..=key_end]);
                if key_end == input.len() - 1 {
                    break;
                } else {
                    pos = key_end + 1;
                }
            } else {
                return Err(match input.as_bytes()[pos] {
                    v @ (b'\"' | b'\'') => DottedBareKeyLexError::QuotedKey(v as char),
                    b'.' => DottedBareKeyLexError::InvalidDotChar,
                    v => DottedBareKeyLexError::InvalidChar(v as char),
                });
            }

            match input.as_bytes()[pos] {
                b'.' => {
                    if pos == input.len() - 1 {
                        return Err(DottedBareKeyLexError::InvalidDotChar);
                    } else {
                        // keys.push(Token::Dot);
                        pos += 1;
                    }
                }
                v => return Err(DottedBareKeyLexError::InvalidChar(v as char)),
            }
        }

        Ok(keys)
    }

    #[test]
    fn test_parse_dotted_bare_keys() {
        assert_eq!(parse_dotted_bare_keys("name"), Ok(vec!["name"]));
        assert_eq!(
            parse_dotted_bare_keys("physical.color"),
            Ok(vec!["physical", "color"])
        );
        assert_eq!(
            parse_dotted_bare_keys("physical.color"),
            Ok(vec!["physical", "color"])
        );
        assert_eq!(
            parse_dotted_bare_keys("invalid..joined"),
            Err(DottedBareKeyLexError::InvalidDotChar)
        );
        assert_eq!(
            parse_dotted_bare_keys(".invalid.joined"),
            Err(DottedBareKeyLexError::InvalidDotChar)
        );
        assert_eq!(
            parse_dotted_bare_keys("invalid.joined."),
            Err(DottedBareKeyLexError::InvalidDotChar)
        );
        assert_eq!(
            parse_dotted_bare_keys("error.\"quoted key\""),
            Err(DottedBareKeyLexError::QuotedKey('\"'))
        );
        assert_eq!(
            parse_dotted_bare_keys("error.\'quoted key\'"),
            Err(DottedBareKeyLexError::QuotedKey('\''))
        );
        assert_eq!(
            parse_dotted_bare_keys("a-zA-Z0-9-_.*invalid*"),
            Err(DottedBareKeyLexError::InvalidChar('*'))
        );
        assert_eq!(
            parse_dotted_bare_keys("a. b .c"),
            Err(DottedBareKeyLexError::InvalidChar(' '))
        );
    }
}

pub(crate) trait TomlValueHelper<'a> {
    fn get_str(&self, name: &str) -> Result<Option<&'a str>, ConfigError>;
    fn get_i64(&self, name: &str) -> Result<Option<i64>, ConfigError>;
    fn get_string_or_i64(&self, name: &str) -> Result<Option<String>, ConfigError>;
    fn get_table(&self, name: &str) -> Result<Option<&'a Table>, ConfigError>;
    fn get_array(&self, name: &str) -> Result<Option<&'a [Value]>, ConfigError>;
}

#[derive(Debug)]
pub(super) struct ExtraMetaData(Table, ExtraMetadataSource);

impl ExtraMetaData {
    pub(super) fn new(source: &ExtraMetadataSource) -> Result<Self, Error> {
        match source {
            ExtraMetadataSource::File(p, branch) => {
                let annot: Option<PathBuf> = Some(p.clone());
                let toml = fs::read_to_string(p)?
                    .parse::<Value>()
                    .map_err(|e| FileAnnotatedError(annot.clone(), e))?;
                let table = Self::convert_toml_txt_to_table(&toml, branch)
                    .map_err(|e| FileAnnotatedError(annot, e))?;
                Ok(Self(table.clone(), source.clone()))
            }
            ExtraMetadataSource::Text(text) => {
                let annot: Option<PathBuf> = None;
                let toml = text
                    .parse::<Value>()
                    .map_err(|e| FileAnnotatedError(annot.clone(), e))?;
                let table = Self::convert_toml_txt_to_table(&toml, &None as &_)
                    .map_err(|e| FileAnnotatedError(annot, e))?;
                Ok(Self(table.clone(), source.clone()))
            }
        }
    }

    fn convert_toml_txt_to_table<'a>(
        toml: &'a Value,
        branch: &Option<String>,
    ) -> Result<&'a Table, ConfigError> {
        let root = toml
            .as_table()
            .ok_or(ConfigError::WrongType(".".to_string(), "table"))?;

        if let Some(branch) = branch {
            toml_dotted_bare_key_parser::parse_dotted_bare_keys(branch.as_ref())
                .map_err(|e| ConfigError::WrongBranchPathOfToml(branch.clone(), e))?
                .iter()
                .fold(Some(root), |table, key| {
                    table.and_then(|v| v.get(*key).and_then(|v| v.as_table()))
                })
                .ok_or(ConfigError::BranchPathNotFoundInToml(branch.to_string()))
        } else {
            Ok(root)
        }
    }
}

pub(super) struct MetadataConfig<'a> {
    metadata: &'a Table,
    branch_path: Option<String>,
}

impl<'a> MetadataConfig<'a> {
    pub fn new(metadata: &'a Table, branch_path: Option<String>) -> Self {
        Self {
            metadata,
            branch_path: branch_path.map(|v| v.to_string()),
        }
    }

    pub fn new_from_extra_metadata(extra_metadata: &'a ExtraMetaData) -> Self {
        Self::new(
            &extra_metadata.0,
            match &extra_metadata.1 {
                ExtraMetadataSource::File(_, Some(branch)) => Some(branch.clone()),
                _ => None,
            },
        )
    }

    pub fn new_from_manifest(manifest: &'a Manifest) -> Result<Option<Self>, Error> {
        let pkg = manifest
            .package
            .as_ref()
            .ok_or(ConfigError::Missing("package".to_string()))?;
        let metadata = pkg.metadata.as_ref();
        if metadata.is_none() {
            return Ok(None);
        }

        let metadata = metadata.unwrap().as_table().ok_or(ConfigError::WrongType(
            "package.metadata".to_string(),
            "table",
        ))?;
        let metadata = metadata
            .iter()
            .find(|(name, _)| name.as_str() == "azsphere")
            .ok_or(ConfigError::Missing(
                "package.metadata.azsphere".to_string(),
            ))?
            .1
            .as_table()
            .ok_or(ConfigError::WrongType(
                "package.metadata.azsphere".to_string(),
                "table",
            ))?;

        Ok(Some(Self {
            metadata: metadata,
            branch_path: Some("package.metadata.azsphere".to_string()),
        }))
    }

    fn create_config_error(&self, name: &str, type_name: &'static str) -> ConfigError {
        let toml_path = self
            .branch_path
            .as_ref()
            .map(|v| [v, name].join("."))
            .unwrap_or(name.to_string());
        ConfigError::WrongType(toml_path, type_name)
    }
}

impl<'a> TomlValueHelper<'a> for MetadataConfig<'a> {
    fn get_str(&self, name: &str) -> Result<Option<&'a str>, ConfigError> {
        self.metadata
            .get(name)
            .map(|val| match val {
                Value::String(v) => Ok(Some(v.as_str())),
                _ => Err(self.create_config_error(name, "string")),
            })
            .unwrap_or(Ok(None))
    }

    fn get_i64(&self, name: &str) -> Result<Option<i64>, ConfigError> {
        self.metadata
            .get(name)
            .map(|val| match val {
                Value::Integer(v) => Ok(Some(*v)),
                _ => Err(self.create_config_error(name, "integer")),
            })
            .unwrap_or(Ok(None))
    }

    fn get_string_or_i64(&self, name: &str) -> Result<Option<String>, ConfigError> {
        self.metadata
            .get(name)
            .map(|val| match val {
                Value::String(v) => Ok(Some(v.clone())),
                Value::Integer(v) => Ok(Some(v.to_string())),
                _ => Err(self.create_config_error(name, "string or integer")),
            })
            .unwrap_or(Ok(None))
    }

    fn get_table(&self, name: &str) -> Result<Option<&'a Table>, ConfigError> {
        self.metadata
            .get(name)
            .map(|val| match val {
                Value::Table(v) => Ok(Some(v)),
                _ => Err(self.create_config_error(name, "string or integer")),
            })
            .unwrap_or(Ok(None))
    }

    fn get_array(&self, name: &str) -> Result<Option<&'a [Value]>, ConfigError> {
        self.metadata
            .get(name)
            .map(|val| match val {
                Value::Array(v) => Ok(Some(v.as_slice())),
                _ => Err(self.create_config_error(name, "array")),
            })
            .unwrap_or(Ok(None))
    }
}

pub(super) struct CompoundMetadataConfig<'a> {
    config: &'a [MetadataConfig<'a>],
}

impl<'a> CompoundMetadataConfig<'a> {
    pub(super) fn new(config: &'a [MetadataConfig<'a>]) -> Self {
        Self { config }
    }

    fn get<T, F>(&self, func: F) -> Result<Option<T>, ConfigError>
    where
        F: Fn(&MetadataConfig<'a>) -> Result<Option<T>, ConfigError>,
    {
        for item in self.config.iter().rev() {
            match func(&item) {
                v @ (Ok(Some(_)) | Err(_)) => return v,
                Ok(None) => continue,
            }
        }
        Ok(None)
    }
}

impl<'a> TomlValueHelper<'a> for CompoundMetadataConfig<'a> {
    fn get_str(&self, name: &str) -> Result<Option<&'a str>, ConfigError> {
        self.get(|v| v.get_str(name))
    }

    fn get_i64(&self, name: &str) -> Result<Option<i64>, ConfigError> {
        self.get(|v| v.get_i64(name))
    }

    fn get_string_or_i64(&self, name: &str) -> Result<Option<String>, ConfigError> {
        self.get(|v| v.get_string_or_i64(name))
    }

    fn get_table(&self, name: &str) -> Result<Option<&'a Table>, ConfigError> {
        self.get(|v| v.get_table(name))
    }

    fn get_array(&self, name: &str) -> Result<Option<&'a [Value]>, ConfigError> {
        self.get(|v| v.get_array(name))
    }
}

#[cfg(test)]
mod test {
    use cargo_toml::Value;
    use toml::toml;

    use super::*;

    #[test]
    fn test_metadata_config() {
        let metadata = toml! {
            str = "str"
            int = 256
            table = { int = 128 }
            array = [ 1, 2 ]
        };
        let metadata_config = MetadataConfig {
            metadata: metadata.as_table().unwrap(),
            branch_path: None,
        };

        assert_eq!(metadata_config.get_str("str").unwrap(), Some("str"));
        assert_eq!(metadata_config.get_i64("int").unwrap(), Some(256));
        assert_eq!(
            metadata_config.get_string_or_i64("str").unwrap(),
            Some("str".to_string())
        );
        assert_eq!(
            metadata_config.get_string_or_i64("int").unwrap(),
            Some("256".to_string())
        );
        assert_eq!(
            metadata_config.get_table("table").unwrap(),
            "int = 128".parse::<Value>().unwrap().as_table()
        );
        assert_eq!(
            metadata_config.get_array("array").unwrap().unwrap(),
            [Value::Integer(1), Value::Integer(2)]
        );

        assert_eq!(metadata_config.get_str("not-exist").unwrap(), None);
        assert!(matches!(
            metadata_config.get_str("int"),
            Err(ConfigError::WrongType(v, "string")) if v == "int".to_string()
        ));
        assert!(matches!(
            metadata_config.get_string_or_i64("array"),
            Err(ConfigError::WrongType(v, "string or integer")) if v == "array".to_string()
        ));

        let metadata_config = MetadataConfig {
            metadata: metadata.as_table().unwrap(),
            branch_path: Some("branch".to_string()),
        };
        assert!(matches!(
            metadata_config.get_str("int"),
            Err(ConfigError::WrongType(v, "string")) if v == "branch.int".to_string()
        ));
        assert!(matches!(
            metadata_config.get_string_or_i64("array"),
            Err(ConfigError::WrongType(v, "string or integer")) if v == "branch.array".to_string()
        ));
    }

    #[test]
    fn test_compound_metadata_config() {
        let metadata = [
            toml! {
                a = 1
                b = 2
            },
            toml! {
                b = 3
                c = 4
            },
        ];
        let metadata_config = metadata
            .iter()
            .map(|v| MetadataConfig {
                metadata: v.as_table().unwrap(),
                branch_path: None,
            })
            .collect::<Vec<_>>();
        let metadata = CompoundMetadataConfig {
            config: metadata_config.as_slice(),
        };
        assert_eq!(metadata.get_i64("a").unwrap(), Some(1));
        assert_eq!(metadata.get_i64("b").unwrap(), Some(3));
        assert_eq!(metadata.get_i64("c").unwrap(), Some(4));
        assert_eq!(metadata.get_i64("not-exist").unwrap(), None);
    }
}
