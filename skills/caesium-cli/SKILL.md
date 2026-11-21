---
name: caesium-cli
description: Use when compressing, resizing, converting, inspecting, or batch-optimizing JPEG, PNG, WebP, TIFF, or GIF images from the terminal, in CI pipelines, via stdin/stdout pipes, or through MCP and stateful agent protocols with caesium-cli.
---

# Caesium CLI (`caesium-cli`)

`caesium-cli` is a subcommand-driven Rust CLI, TUI, stateful NDJSON agent session, and native MCP server backed by `libcaesium`. Always use subcommands (`caesium-cli <cmd>`)—never guess legacy `caesiumclt` flags or wrap it in bash `find`/`mktemp` loops. Recursion, atomic writes, filtering, dry-runs, receipts, retries, and JSON/NDJSON are built in.

## Quick Reference

| Job | Command |
| --- | --- |
| Capabilities & schemas | `caesium-cli capabilities --json` · `caesium-cli schema <options\|batch\|report\|all>` |
| Inspect metadata/EXIF | `caesium-cli inspect <paths...> --json` |
| Preview (zero writes) | `caesium-cli preview <paths...> [flags] --json` |
| Dry-run plan | `caesium-cli compress <paths...> -o <dir> --dry-run --json` |
| Compress batch | `caesium-cli compress <paths...> -o <dir> --preset web --keep-structure --json` |
| Target size ceiling | `caesium-cli compress <dir> -o <out> --max-size 250KB --resize long:1600 --receipt job.json --json` |
| In-place overwrite | `caesium-cli compress <dir> --same-folder --overwrite --format webp --no-skip-if-bigger --json` |
| Convert format | `caesium-cli convert <paths...> --to webp -o <out> --json` |
| Retry failed items | `caesium-cli retry job.json --json` |
| Stdin -> stdout pipe | `caesium-cli compress - -o - --format webp --webp-quality 75 < in.png > out.webp` |
| Multi-variant batch | `caesium-cli batch <spec.json\|-> --json` (uses `{"tasks": [...]}`) |
| Stateful agent session | `caesium-cli agent` (NDJSON: `add`, `apply_preset`, `set_options`, `preview`, `dry_run`, `compress`, `retry_failed`, `quit`) |
| Native MCP stdio | `caesium-cli mcp` (tools: `caesium_inspect`, `caesium_preview`, `caesium_compress`, `caesium_convert`, `caesium_batch`, `caesium_presets`, `caesium_retry`) |
| Interactive TUI | `caesium-cli tui [paths...] -o <dir>` |
| Health check | `caesium-cli doctor --json` |

## Agentic Rules

1. Run `inspect --json` / `preview --json` before tuning quality or byte ceilings.
2. Run `--dry-run --json` before `--same-folder --overwrite` or `--move-original delete`.
3. Use `--json` (envelope) or `--ndjson` (live `start`/`item`/`complete` stream) and `--receipt <path>` on batches; recover partial failures with `caesium-cli retry <receipt> --json`.
4. Omit `--json`/`--ndjson` when piping binary bytes via `compress - -o -`.
5. Run `caesium-cli schema batch` before constructing multi-task `BatchJobSpec` JSON.

## Exit Codes

`0` success · `1` partial failure (run `retry`) · `2` invalid args/schema (read `error.hint`) · `3` no matching inputs · `4` `OVERWRITE_REFUSED` (add `--overwrite`, `--suffix`, or `-o <dir>`) · `5` fatal I/O or codec error (`doctor --json`).

## Common Mistakes

| Mistake | Fix |
| --- | --- |
| Omitting subcommand (`caesium-cli -q 80 ...`) | Always write `caesium-cli compress ...` |
| `--same-folder-as-input`, `--overwrite all`, `--no-enlarge`, `--progressive` | Use `--same-folder`, `--overwrite`, `--do-not-enlarge` (default on; `--allow-enlarge` disables), `--jpeg-progressive` (default on; `--no-jpeg-progressive` disables) |
| Bash `find`/`mktemp` loops | Built-in recursive scan, atomic writes, `--json`, `--receipt`, `retry` |
| `variants` in batch JSON or `compress_image` in MCP | Batch uses `{"tasks": [...]}`; MCP tools are `caesium_*` |
| Missing `--no-skip-if-bigger` on forced `compress --format` | Use `convert --to <fmt>` or pass `--no-skip-if-bigger` |

Full flags, presets, batch/agent JSON examples, and MCP resources/prompts: [`references/commands-and-schemas.md`](references/commands-and-schemas.md).
