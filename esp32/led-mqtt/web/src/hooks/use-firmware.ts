import { useCallback, useEffect, useState } from 'react'
import { FIRMWARE_MANIFEST_URL, type FirmwareManifest } from '@/lib/device'

const RECHECK_MS = 30 * 60 * 1000

/**
 * Latest published firmware, read from the manifest the Pages workflow deploys.
 * Checks on load, every 30 minutes while the page is open, and on demand via check().
 */
export function useFirmware() {
  const [latest, setLatest] = useState<FirmwareManifest | null>(null)
  const [checking, setChecking] = useState(false)
  const [checkedAt, setCheckedAt] = useState<number | null>(null)
  const [error, setError] = useState<string | null>(null)

  const check = useCallback(async () => {
    setChecking(true)
    try {
      // Query string skips the Pages CDN cache so a fresh deploy shows up right away
      const res = await fetch(`${FIRMWARE_MANIFEST_URL}?t=${Date.now()}`, { cache: 'no-store' })
      if (!res.ok) throw new Error(res.status === 404 ? 'No firmware published yet' : `HTTP ${res.status}`)
      setLatest((await res.json()) as FirmwareManifest)
      setError(null)
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e))
    } finally {
      setChecking(false)
      setCheckedAt(Date.now())
    }
  }, [])

  useEffect(() => {
    void check()
    const id = setInterval(() => void check(), RECHECK_MS)
    return () => clearInterval(id)
  }, [check])

  /** Absolute URL of the latest .bin (the manifest names it relative to itself) */
  const binUrl = latest ? new URL(latest.file, FIRMWARE_MANIFEST_URL).href : null

  return { latest, binUrl, checking, checkedAt, error, check }
}
