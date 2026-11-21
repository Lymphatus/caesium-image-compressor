# Caesium CLI — Detailed Commands, Schemas & Protocols Reference

## Compression & Tuning Flags (`compress`, `convert`, `preview`, `preset save`)

### Global Output Flags
- `--json` — Pretty JSON envelope `{ ok, schema_version, command, result, warnings, error }`
- `--ndjson` — Live newline-delimited JSON events (`start`, `item`, `complete`)
- `--quiet` — Suppress non-error output
- `--output-format <human|json|ndjson|quiet>`

### Codec & Mode Flags
- `--preset <name>` — Built-ins: `gui-default`, `web`, `lossless`, `thumbnail`, `social`, `email`, `archive`, `aggressive`
- `-q, --quality <0-100>` — Universal quality across JPEG, PNG, WebP, GIF
- `--max-size <size>` — Size mode ceiling: bytes (`250000`), `KB` (`250KB`), `MB` (`2MB`), or `%` (`50%`)
- `--strict-size` — Fail if target ceiling cannot be met (default returns smallest possible)
- `--lossless` — Lossless optimization (`libcaesium` `optimize = true`)
- `--keep-metadata` / `--strip-metadata` — Keep or strip EXIF/ICC metadata (default: keep)
- `-f, --format <same|jpg|png|webp|tiff|gif>` — Output format conversion
- `--jpeg-quality <0-100>` (default 80)
- `--jpeg-chroma <auto|4:4:4|4:2:2|4:2:0|4:1:1>` (default `auto`)
- `--jpeg-progressive` / `--no-jpeg-progressive` (default: progressive enabled)
- `--png-quality <0-100>` (default 80)
- `--png-opt-level <0-6>` (default 3, used in lossless PNG optimization)
- `--png-zopfli` — Force Zopfli deflate (slow, maximum lossless PNG savings)
- `--webp-quality <0-100>` (default 60)
- `--gif-quality <0-100>` (default 80)
- `--tiff-method <uncompressed|lzw|deflate|packbits>` (default `lzw`)
- `--tiff-deflate-level <fast|balanced|best>` (default `balanced`)

### EXIF-Aware Resize Flags
- `--resize <spec>` — Compact specs:
  - `1920x1080` (exact dimensions)
  - `50%` or `50%x75%` (percentage)
  - `long:1600` (long edge)
  - `short:600` (short edge)
  - `w:800` (fixed width, preserve aspect ratio)
  - `h:600` (fixed height, preserve aspect ratio)
- Explicit flags: `--width <px>`, `--height <px>`, `--long-edge <px>`, `--short-edge <px>`, `--resize-percent <1-999>`
- `--do-not-enlarge` (default) / `--allow-enlarge`

### Discovery, Filtering & Output Flags
- `--from-list <file|->` — Newline-delimited paths (or `-` for stdin)
- `-r, --recursive` (default) / `--no-recursive`
- `--max-input-size <size|0>` — Input size guard (default `500MB`; `0` = unlimited)
- `--skip-size-gt <size>`, `--skip-size-eq <size>`, `--skip-size-lt <size>` — Advanced Import size filter
- `--regex <pattern>` — Filename regex filter
- `-o, --output <dir|file|->`, `--same-folder`, `--suffix <str>`, `--keep-structure`
- `-y, --overwrite` — Required to overwrite originals in-place (otherwise exits `4` with `OVERWRITE_REFUSED`)
- `--skip-if-bigger` (default on `compress`) / `--no-skip-if-bigger` (default on `convert`)
- `--move-original <none|trash|delete>`
- `--keep-dates`, `--keep-modified`, `--keep-accessed`
- `-j, --threads <n>` (`0` = auto CPU cores)
- `--dry-run` — Output complete execution plan with zero disk writes
- `--receipt <path>` — Write job receipt JSON for `caesium-cli retry <path>`
- `--post-action <none|open-folder|sleep|shutdown>`, `--exec <cmd>`

## Declarative Batch Manifest (`caesium-cli batch <spec.json|->`)

Inspect live schema with `caesium-cli schema batch`. Example generating two variants from one source in a single invocation:

```json
{
  "threads": 4,
  "dry_run": false,
  "tasks": [
    {
      "name": "thumbnail-webp",
      "inputs": ["./assets/hero.png"],
      "output": "./assets/out/hero-thumb.webp",
      "options": {
        "format": "webp",
        "webp_quality": 75,
        "skip_if_bigger": false,
        "resize": {
          "mode": "long_edge",
          "edge": 400,
          "do_not_enlarge": true
        }
      }
    },
    {
      "name": "hero-jpeg",
      "inputs": ["./assets/hero.png"],
      "output": "./assets/out/hero.jpg",
      "options": {
        "format": "jpg",
        "jpeg_quality": 80,
        "jpeg_progressive": true,
        "skip_if_bigger": false
      }
    }
  ]
}
```

Run from stdin:

```bash
caesium-cli batch - --json < batch.json
```

## Stateful NDJSON Agent Session (`caesium-cli agent`)

Send one JSON object per line on stdin; read one JSON object per line on stdout (first emitted line is a `ready` handshake event with capabilities):

```json
{"id":"1","op":"add","paths":["./assets/raw"],"recursive":true}
{"id":"2","op":"apply_preset","name":"web"}
{"id":"3","op":"set_options","options":{"max_output_size":{"value":250,"unit":"kilobytes"},"mode":"size"}}
{"id":"4","op":"preview"}
{"id":"5","op":"dry_run","output":"./assets/out"}
{"id":"6","op":"compress","output":"./assets/out","receipt":"job.json","threads":4}
{"id":"7","op":"retry_failed"}
{"id":"8","op":"quit"}
```

Supported ops: `capabilities`, `add`, `list`, `remove`, `clear`, `get_options`, `set_options`, `apply_preset`, `list_presets`, `inspect`, `preview`, `dry_run`, `compress`, `retry_failed`, `status`, `quit`.

## Native MCP Server (`caesium-cli mcp`)

JSON-RPC 2.0 over stdio (auto-detects newline-delimited JSON and `Content-Length` framing).

### Client Config
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

### MCP Tools
- `caesium_inspect` — Inspect image files or directories without mutating disk
- `caesium_preview` — In-memory compression preview with exact byte savings
- `caesium_compress` — Full compression, resize, target size, filters, dry-run, receipts
- `caesium_convert` — Format conversion (`to`: `jpg|png|webp|tiff|gif`)
- `caesium_batch` — Execute a `BatchJobSpec` object
- `caesium_presets` — `list`, `get`, `save`, `delete` presets
- `caesium_retry` — Retry failed items from a saved receipt JSON

### MCP Resources & Prompts
- Resources: `caesium://capabilities`, `caesium://presets`, `caesium://schema/options`, `caesium://schema/batch`
- Prompts: `optimize-for-web`, `fit-under-size-budget`, `batch-convert-assets`
