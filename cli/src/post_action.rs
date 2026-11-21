use crate::error::{CaesiumCliError, CliResult, ErrorCode};
use crate::options::PostCompressionAction;
use std::path::Path;
use std::process::Command;

/// Executes GUI-equivalent post-compression actions and optional custom shell hooks.
pub fn run_post_action(
    action: PostCompressionAction,
    output_folder: Option<&Path>,
    post_exec: Option<&str>,
) -> CliResult<()> {
    if let Some(cmd) = post_exec {
        if !cmd.trim().is_empty() {
            let status = if cfg!(target_os = "windows") {
                Command::new("cmd").args(["/C", cmd]).status()
            } else {
                Command::new("sh").args(["-c", cmd]).status()
            }
            .map_err(|e| {
                CaesiumCliError::new(
                    ErrorCode::IoError,
                    format!("Failed to execute --exec hook: {e}"),
                )
            })?;
            if !status.success() {
                return Err(CaesiumCliError::new(
                    ErrorCode::IoError,
                    format!("--exec hook exited with status {status}"),
                ));
            }
        }
    }

    match action {
        PostCompressionAction::None => Ok(()),
        PostCompressionAction::OpenFolder => {
            if let Some(dir) = output_folder {
                open_folder(dir)?;
            }
            Ok(())
        }
        PostCompressionAction::Sleep => trigger_sleep(),
        PostCompressionAction::Shutdown => trigger_shutdown(),
    }
}

fn open_folder(dir: &Path) -> CliResult<()> {
    #[cfg(target_os = "macos")]
    let mut cmd = {
        let mut c = Command::new("open");
        c.arg(dir);
        c
    };
    #[cfg(target_os = "windows")]
    let mut cmd = {
        let mut c = Command::new("explorer");
        c.arg(dir);
        c
    };
    #[cfg(all(unix, not(target_os = "macos")))]
    let mut cmd = {
        let mut c = Command::new("xdg-open");
        c.arg(dir);
        c
    };

    cmd.spawn().map_err(|e| {
        CaesiumCliError::new(
            ErrorCode::IoError,
            format!("Failed to open output folder {}: {e}", dir.display()),
        )
    })?;
    Ok(())
}

fn trigger_sleep() -> CliResult<()> {
    #[cfg(target_os = "windows")]
    let status = Command::new("rundll32.exe")
        .args(["powrprof.dll,SetSuspendState", "0,1,0"])
        .status();
    #[cfg(target_os = "macos")]
    let status = Command::new("pmset").arg("sleepnow").status();
    #[cfg(all(unix, not(target_os = "macos")))]
    let status = Command::new("systemctl").arg("suspend").status();

    status.map_err(|e| CaesiumCliError::new(ErrorCode::IoError, format!("Sleep failed: {e}")))?;
    Ok(())
}

fn trigger_shutdown() -> CliResult<()> {
    #[cfg(target_os = "windows")]
    let status = Command::new("shutdown").arg("/s").status();
    #[cfg(not(target_os = "windows"))]
    let status = Command::new("shutdown").args(["-h", "now"]).status();

    status
        .map_err(|e| CaesiumCliError::new(ErrorCode::IoError, format!("Shutdown failed: {e}")))?;
    Ok(())
}
