# Caesium CLI (`caesium-cli`)

Extensive command-line interface, interactive Terminal User Interface (TUI), and agent-native automation layer (JSON/NDJSON, stateful NDJSON protocol, and native Model Context Protocol stdio server) for **Caesium Image Compressor**, powered directly by `libcaesium 0.17.4` with zero Qt/GUI display dependencies.

## Features (Full Desktop GUI Parity + Agentic Superpowers)

- **All Formats:** JPEG (`mozjpeg`), PNG (`oxipng`, `lodepng`, `imagequant`, Zopfli), WebP, TIFF (`uncompressed`, `lzw`, `deflate`, `packbits`), and GIF (`gifsicle`)
- **Compression Modes:** Per-format quality (`--jpeg-quality`, `--jpeg-chroma`, `--no-jpeg-progressive`, `--png-quality`, `--png-opt-level`, `--png-zopfli`, `--webp-quality`, `--gif-quality`, `--tiff-method`, `--tiff-deflate-level`), lossless (`--lossless`), and target maximum output size (`--max-size 250KB|2MB|50%`)
- **EXIF-Aware Resizing:** Dimensions (`1920x1080`), percentage (`50%`, `50%x75%`), long edge (`long:1600`), short edge (`short:600`), fixed width (`w:800`), fixed height (`h:600`), plus `--do-not-enlarge` / `--allow-enlarge`
- **Format Conversion:** Convert between JPG, PNG, WebP, TIFF, and GIF (`--format` or `convert --to`)
- **Batch Discovery & Advanced Import:** Recursive folder scanning, `--from-list <file|->`, size filters (`--skip-size-gt|eq|lt`), filename regex (`--regex`), directory structure preservation (`--keep-structure`), suffixing (`--suffix`), file date preservation (`--keep-dates`), original trash/delete (`--move-original trash|delete`), and in-place overwrite protection (`--overwrite`)
- **Zero-Mutation Inspection & Planning:** `inspect`, in-memory `preview`, and `--dry-run`
- **Resilient Parallel Execution:** Multithreaded via `rayon` (`-j/--threads`), atomic temp-file writes, job receipts (`--receipt`), and `retry` for failed items only
- **Stdin / Stdout Pipelines:** `caesium-cli compress - -o - --format webp -q 75 < in.png > out.webp`
- **Presets & GUI Config Import:** 8 built-in presets (`gui-default`, `web`, `lossless`, `thumbnail`, `social`, `email`, `archive`, `aggressive`), custom preset persistence, and `config import-gui` to import Caesium desktop GUI's `QSettings` INI
- **Interactive TUI:** `caesium-cli tui [PATHS...]` (or bare `caesium-cli` in a TTY) — queue table, 3-tab settings panel (Compression / Resize / Output), live in-memory preview, preset picker, advanced import modal, progress bar with cancel, retry failed, and overwrite guard
- **Agent-Native Interfaces:**
  - Universal `--json` envelope and `--ndjson` live event streaming across commands
  - Self-describing `capabilities` and Draft 2020-12 `schema` introspection
  - Declarative multi-task `batch <spec.json|->` execution
  - Stateful NDJSON interactive protocol (`caesium-cli agent`)
  - Native **MCP (Model Context Protocol)** JSON-RPC 2.0 stdio server (`caesium-cli mcp`) with 7 tools, 4 resources, and 3 workflow prompts
  - Companion AI skill at [`../skills/caesium-cli/SKILL.md`](../skills/caesium-cli/SKILL.md)

## Build & Install

Requires Rust 1.80+ (`cargo`).

```bash
# Build release binary
cargo build --manifest-path cli/Cargo.toml --release

# Install into ~/.cargo/bin
cargo install --path cli

# Verify codecs & environment
caesium-cli doctor
```

## Quick Examples

```bash
# Launch interactive TUI on a folder
caesium-cli tui ./photos -o ./optimized

# One-shot web optimization with structured JSON output
caesium-cli compress ./photos -o ./dist --preset web --keep-structure --json

# Fit every image under 250 KB, resize long edge to 1600px, save receipt for retry
caesium-cli compress ./raw -o ./out --max-size 250KB --resize long:1600 --receipt job.json --ndjson

# Retry only failed items from a prior run
caesium-cli retry job.json --json

# Inspect metadata, EXIF orientation, dimensions, and SHA-256
caesium-cli inspect ./photos --json

# In-memory preview without writing any files
caesium-cli preview ./hero.png --format webp --webp-quality 75 --json

# Stream bytes over stdin -> stdout
caesium-cli compress - -o - --format webp --webp-quality 75 < input.png > output.webp

# Dry-run before an in-place overwrite
caesium-cli compress ./photos --same-folder --dry-run --json
caesium-cli compress ./photos --same-folder --overwrite --json

# Generate shell completions
caesium-cli completions bash > ~/.local/share/bash-completion/completions/caesium-cli
```

## Connecting AI Agents

### 1. Native MCP Server (`caesium-cli mcp`)

Add to your MCP client configuration (Claude Desktop, Antigravity, Cursor, Codex, etc.):

```json
{
  "mcpServers": {
    "caesium": {
      "command": "caesium-cli",
      "args": ["mcp"]
    }
  }
}
```

Exposed tools: `caesium_inspect`, `caesium_preview`, `caesium_compress`, `caesium_convert`, `caesium_batch`, `caesium_presets`, `caesium_retry`.
Exposed resources: `caesium://capabilities`, `caesium://presets`, `caesium://schema/options`, `caesium://schema/batch`.

### 2. Stateful NDJSON Agent Session (`caesium-cli agent`)

```json
{"id":"1","op":"add","paths":["./photos"],"recursive":true}
{"id":"2","op":"apply_preset","name":"web"}
{"id":"3","op":"preview"}
{"id":"4","op":"compress","output":"./dist","threads":4}
{"id":"5","op":"quit"}
```

### 3. Deterministic Exit Codes

| Exit Code | Meaning |
| --- | --- |
| `0` | Success |
| `1` | Partial failure (one or more files failed; see `items` or `retry`) |
| `2` | Invalid arguments, option value, or protocol error |
| `3` | No matching input images found |
| `4` | In-place overwrite refused (`--overwrite`, `--suffix`, or `-o <DIR>` required) |
| `5` | Fatal runtime / I/O / codec error |
