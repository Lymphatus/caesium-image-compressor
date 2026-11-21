use schemars::JsonSchema;
use serde::{Deserialize, Serialize};
use std::fmt;

/// Deterministic process exit codes for CLI and agent integrations.
pub mod exit_codes {
    pub const SUCCESS: i32 = 0;
    pub const PARTIAL_FAILURE: i32 = 1;
    pub const INVALID_ARGS: i32 = 2;
    pub const NO_INPUTS: i32 = 3;
    pub const OVERWRITE_REFUSED: i32 = 4;
    pub const RUNTIME_ERROR: i32 = 5;
}

/// Stable machine-readable error codes emitted in JSON/NDJSON/MCP responses.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, JsonSchema)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
pub enum ErrorCode {
    InvalidOption,
    InvalidInput,
    NoMatchingInputs,
    UnsupportedFormat,
    FileTooBig,
    OverwriteRefused,
    PresetNotFound,
    PresetProtected,
    ConfigError,
    IoError,
    CompressionFailed,
    ConversionFailed,
    PartialFailure,
    ProtocolError,
    Canceled,
}

impl ErrorCode {
    pub fn exit_code(self) -> i32 {
        match self {
            ErrorCode::InvalidOption | ErrorCode::InvalidInput | ErrorCode::ProtocolError => {
                exit_codes::INVALID_ARGS
            }
            ErrorCode::NoMatchingInputs => exit_codes::NO_INPUTS,
            ErrorCode::OverwriteRefused => exit_codes::OVERWRITE_REFUSED,
            ErrorCode::PartialFailure => exit_codes::PARTIAL_FAILURE,
            ErrorCode::UnsupportedFormat
            | ErrorCode::FileTooBig
            | ErrorCode::PresetNotFound
            | ErrorCode::PresetProtected
            | ErrorCode::ConfigError
            | ErrorCode::IoError
            | ErrorCode::CompressionFailed
            | ErrorCode::ConversionFailed
            | ErrorCode::Canceled => exit_codes::RUNTIME_ERROR,
        }
    }
}

/// Structured error with machine-readable code and actionable recovery hint.
#[derive(Debug, Clone, Serialize, Deserialize, JsonSchema)]
pub struct CaesiumCliError {
    pub code: ErrorCode,
    pub message: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub hint: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub path: Option<String>,
}

impl CaesiumCliError {
    pub fn new(code: ErrorCode, message: impl Into<String>) -> Self {
        Self {
            code,
            message: message.into(),
            hint: None,
            path: None,
        }
    }

    pub fn with_hint(mut self, hint: impl Into<String>) -> Self {
        self.hint = Some(hint.into());
        self
    }

    pub fn with_path(mut self, path: impl Into<String>) -> Self {
        self.path = Some(path.into());
        self
    }

    pub fn exit_code(&self) -> i32 {
        self.code.exit_code()
    }
}

impl fmt::Display for CaesiumCliError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "[{:?}] {}", self.code, self.message)?;
        if let Some(path) = &self.path {
            write!(f, " ({})", path)?;
        }
        if let Some(hint) = &self.hint {
            write!(f, "\nHint: {}", hint)?;
        }
        Ok(())
    }
}

impl std::error::Error for CaesiumCliError {}

impl From<std::io::Error> for CaesiumCliError {
    fn from(err: std::io::Error) -> Self {
        Self::new(ErrorCode::IoError, err.to_string())
    }
}

impl From<serde_json::Error> for CaesiumCliError {
    fn from(err: serde_json::Error) -> Self {
        Self::new(ErrorCode::InvalidOption, format!("JSON error: {err}"))
    }
}

pub type CliResult<T> = Result<T, CaesiumCliError>;
