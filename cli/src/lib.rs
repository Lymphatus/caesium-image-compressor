//! Caesium CLI, interactive TUI, and agent/MCP core library.

pub mod agent;
pub mod cli;
pub mod config;
pub mod engine;
pub mod error;
pub mod inspect;
pub mod mcp;
pub mod options;
pub mod output;
pub mod post_action;
pub mod scanner;
pub mod tui;

pub const APP_NAME: &str = "caesium-cli";
pub const APP_VERSION: &str = env!("CARGO_PKG_VERSION");
pub const SCHEMA_VERSION: &str = "1.0.0";
