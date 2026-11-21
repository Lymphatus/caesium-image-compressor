pub mod state;
pub mod ui;

use crate::error::{CaesiumCliError, CliResult, ErrorCode};
use crate::options::CompressionOptions;
use crossterm::event::{self, Event, KeyEventKind};
use state::TuiApp;
use std::path::PathBuf;
use std::time::Duration;

/// Launches the interactive terminal UI (full Caesium GUI parity in the terminal).
pub fn run_tui(
    initial_paths: Vec<PathBuf>,
    initial_options: CompressionOptions,
    output_dir: Option<PathBuf>,
) -> CliResult<()> {
    let mut app = TuiApp::new(initial_paths, initial_options, output_dir);
    let mut terminal = ratatui::try_init().map_err(|e| {
        CaesiumCliError::new(
            ErrorCode::IoError,
            format!("Failed to initialize terminal UI: {e}"),
        )
        .with_hint("Run inside an interactive TTY, or use non-interactive CLI subcommands (`compress`, `inspect`, `agent`, `mcp`)")
    })?;

    let loop_result = (|| -> std::io::Result<()> {
        while !app.should_quit {
            app.poll_worker();
            terminal.draw(|frame| ui::render(frame, &mut app))?;

            if event::poll(Duration::from_millis(50))? {
                if let Event::Key(key) = event::read()? {
                    if key.kind == KeyEventKind::Press {
                        app.handle_key(key);
                    }
                }
            }
        }
        Ok(())
    })();

    ratatui::restore();
    loop_result.map_err(CaesiumCliError::from)
}
