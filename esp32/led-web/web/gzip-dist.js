// Replace every built file with a .gz version. The ESP32 web server
// automatically serves "file.gz" with Content-Encoding: gzip when "file" is requested.
import { readdirSync, readFileSync, writeFileSync, unlinkSync, statSync } from 'node:fs'
import { join } from 'node:path'
import { gzipSync } from 'node:zlib'

const dir = new URL('../data', import.meta.url).pathname.replace(/^\/([A-Za-z]:)/, '$1')

for (const name of readdirSync(dir)) {
  const file = join(dir, name)
  if (statSync(file).isDirectory() || name.endsWith('.gz')) continue
  const raw = readFileSync(file)
  const gz = gzipSync(raw, { level: 9 })
  writeFileSync(file + '.gz', gz)
  unlinkSync(file)
  console.log(`${name}: ${raw.length} -> ${gz.length} bytes`)
}
