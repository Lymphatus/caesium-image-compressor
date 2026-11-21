use std::process::ExitCode;

fn main() -> ExitCode {
    let code = caesium_cli::cli::run_cli();
    ExitCode::from(code as u8)
}
