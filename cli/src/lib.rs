//! Caesium CLI, interactive TUI, and agent/MCP core library.

pub mod config;
pub mod error;
pub mod options;

pub const APP_NAME: &str = "caesium-cli";
pub const APP_VERSION: &str = env!("CARGO_PKG_VERSION");
pub const SCHEMA_VERSION: &str = "1.0.0";
