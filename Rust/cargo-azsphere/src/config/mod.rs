use cargo_toml::Error as CargoTomlError;
use cargo_toml::Manifest;
use std::path::{Path, PathBuf};
use toml::value::Value;

use crate::error::{ConfigError, Error};
use metadata::{CompoundMetadataConfig, ExtraMetaData, MetadataConfig, TomlValueHelper};

mod metadata;

#[derive(Debug, Clone)]
pub enum ExtraMetadataSource {
    File(PathBuf, Option<String>),
    Text(String),
}

#[derive(Debug)]
pub struct Config {
    manifest: Manifest,
    _manifest_path: PathBuf,
    extra_metadata: Vec<ExtraMetaData>,
}

/// Required fields, retrieved from the app's Cargo.toml
#[derive(Debug)]
pub struct PackageConfig {
    /// Cargo.Toml package.name
    pub name: String,
    /// app manifest filename
    pub app_manifest: String,
    /// ARV version to target
    pub arv: String,
    /// subdir under the HardwareDefinitions directory tree to use (usually, mt3620_rdb)
    pub target_hardware: Option<String>,
    /// base of JSON filename to use for the target (usually, sample_appliance)
    pub target_definition: Option<String>,
    /// extra files to include in the app package
    pub extra_files: Option<Vec<Value>>,
}

/// Parse the app's Cargo.toml, with optional overrides
impl Config {
    pub fn new(path: &Path, extra_metadata: &[ExtraMetadataSource]) -> Result<Self, Error> {
        let manifest_path = path.to_path_buf();
        let extra_metadata = extra_metadata
            .iter()
            .map(|v| ExtraMetaData::new(v))
            .collect::<Result<Vec<_>, _>>()?;
        Manifest::from_path(&path)
            .map(|manifest| Config {
                manifest,
                _manifest_path: manifest_path,
                extra_metadata,
            })
            .map_err(|err| match err {
                CargoTomlError::Io(e) => Error::FileIo(PathBuf::from(path), e),
                _ => Error::CargoToml(err),
            })
    }

    fn get_value_from_config(package_root: &String, variable_name: &str) -> Option<String> {
        let config_file = PathBuf::from(package_root).join(".cargo/config");
        let str = std::fs::read_to_string(&config_file).expect("Error in reading the file");
        let toml = str.parse::<Value>();
        if let Ok(toml) = toml {
            let env = toml["env"].as_table();
            if let Some(env) = env {
                let value = env[variable_name].as_str();
                if let Some(value) = value {
                    let value = value.to_string();
                    return Some(value.replace("\"", ""));
                }
            }
        }
        return None;
    }

    fn get_arv_from_config(package_root: &String, verbose: bool) -> String {
        let arv = Self::get_value_from_config(&package_root, "AZURE_SPHERE_ARV");
        match arv {
            Some(arv) => arv,
            _ => {
                let default_arv = "14".to_string();
                if verbose {
                    println!("WARNING: Using default ARV {}", default_arv);
                }
                default_arv
            }
        }
    }

    pub fn package_config(
        &self,
        package_root: String,
        verbose: bool,
    ) -> Result<PackageConfig, Error> {
        let mut metadata_config = Vec::new();
        let metadata = MetadataConfig::new_from_manifest(&self.manifest)?;
        if let Some(metadata_file) = metadata {
            metadata_config.push(metadata_file);
        } else if verbose {
            println!("No [package.metadata.azsphere] found.  Using defaults.")
        }
        for v in &self.extra_metadata {
            metadata_config.push(MetadataConfig::new_from_extra_metadata(v));
        }
        let metadata = CompoundMetadataConfig::new(metadata_config.as_slice());

        let pkg = self
            .manifest
            .package
            .as_ref()
            .ok_or(ConfigError::Missing("package".to_string()))?;

        let name = metadata
            .get_str("name")?
            .unwrap_or_else(|| pkg.name.as_str());

        let app_manifest = metadata
            .get_str("app_manifest")?
            .unwrap_or_else(|| "app_manifest.json");

        let arv = metadata.get_str("arv")?;
        let arv = if arv.is_none() {
            Self::get_arv_from_config(&package_root, verbose)
        } else {
            arv.unwrap().to_string()
        };

        let extra_files = metadata.get_array("extra_files")?;
        let extra_files = if let Some(files) = extra_files {
            Some(files.to_vec())
        } else {
            None
        };

        let target_definition = metadata.get_str("target_definition")?;
        let target_definition = if target_definition.is_none() {
            Self::get_value_from_config(&package_root, "AZURE_SPHERE_TARGET_DEFINITION")
        } else {
            Some(target_definition.unwrap().to_string())
        };
        let target_definition = if let Some(t) = target_definition {
            Some(t)
        } else {
            None
        };

        let target_hardware = metadata.get_str("target_hardware")?;
        let target_hardware = if target_hardware.is_none() {
            Self::get_value_from_config(&package_root, "AZURE_SPHERE_TARGET_HARDWARE")
        } else {
            Some(target_hardware.unwrap().to_string())
        };

        Ok(PackageConfig {
            name: name.to_string(),
            app_manifest: app_manifest.to_string(),
            arv,
            target_definition,
            target_hardware,
            extra_files,
        })
    }
}
