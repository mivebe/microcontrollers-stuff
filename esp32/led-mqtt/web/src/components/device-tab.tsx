import { useEffect, useState } from 'react'
import {
  CircleAlert,
  CircleCheck,
  Clock,
  Cpu,
  Download,
  Fingerprint,
  Loader2,
  MemoryStick,
  Network,
  Package,
  Pencil,
  RefreshCw,
  RotateCcw,
  Tag,
  Timer,
  Trash2,
  Wifi,
} from 'lucide-react'
import {
  AlertDialog,
  AlertDialogAction,
  AlertDialogCancel,
  AlertDialogContent,
  AlertDialogDescription,
  AlertDialogFooter,
  AlertDialogHeader,
  AlertDialogTitle,
  AlertDialogTrigger,
} from '@/components/ui/alert-dialog'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { cn } from '@/lib/utils'
import type { useFirmware } from '@/hooks/use-firmware'
import {
  type Command,
  type Device,
  type OtaProgress,
  compareVersions,
  formatBytes,
  formatDuration,
  signalQuality,
} from '@/lib/device'

const INTERVALS = [
  { s: 10, label: 'Every 10 seconds' },
  { s: 30, label: 'Every 30 seconds' },
  { s: 60, label: 'Every minute' },
  { s: 300, label: 'Every 5 minutes' },
  { s: 900, label: 'Every 15 minutes' },
]

interface Props {
  device: Device | null
  disabled: boolean
  send: (cmd: Command) => void
  firmware: ReturnType<typeof useFirmware>
  update: (url: string, version: string) => void
  dismissOta: () => void
  forget: () => void
}

export function DeviceTab({ device, disabled, send, firmware, update, dismissOta, forget }: Props) {
  const now = useNow()
  const state = device?.state ?? null
  const stateAt = device?.stateAt ?? null
  // Uptime keeps counting between state reports
  const uptime = state && stateAt ? state.uptime + (now - stateAt) / 1000 : null
  const signal = state ? signalQuality(state.rssi) : null

  const intervals = state && !INTERVALS.some((i) => i.s === state.interval)
    ? [...INTERVALS, { s: state.interval, label: `Every ${state.interval} seconds` }]
    : INTERVALS

  return (
    <div className="grid gap-4">
      <div className="grid grid-cols-2 gap-3">
        <Stat icon={Clock} label="Uptime" value={uptime !== null ? formatDuration(uptime) : '—'} />
        <Stat
          icon={Wifi}
          label="WiFi signal"
          value={state ? `${state.rssi} dBm` : '—'}
          detail={signal?.label}
          extra={signal && <SignalBars bars={signal.bars} />}
        />
        <Stat
          icon={MemoryStick}
          label="Free memory"
          value={state ? formatBytes(state.heap) : '—'}
          detail={state?.minHeap ? `lowest ${formatBytes(state.minHeap)}` : undefined}
        />
        <Stat
          icon={RotateCcw}
          label="Last restart"
          value={state?.reset ? state.reset[0].toUpperCase() + state.reset.slice(1) : '—'}
          detail={state?.reset === 'watchdog' || state?.reset === 'crash' ? 'recovered automatically' : undefined}
        />
      </div>

      <Card>
        <CardHeader>
          <CardTitle className="flex items-center gap-2">
            <Cpu className="size-4" /> Hardware & network
          </CardTitle>
          <CardDescription>
            {stateAt ? `Reported ${formatDuration((now - stateAt) / 1000)} ago` : 'No report yet'}
          </CardDescription>
        </CardHeader>
        <CardContent>
          <dl className="grid gap-3 text-sm">
            <Row icon={Tag} label="Device id" value={device?.id} mono />
            <Row icon={Package} label="Firmware" value={state?.fw && `v${state.fw}`} />
            <Row icon={Cpu} label="Chip" value={state?.chip && `${state.chip} @ ${state.cpuMhz} MHz`} />
            <Row icon={Wifi} label="Network" value={state?.ssid} />
            <Row icon={Network} label="Local IP" value={state?.ip} mono />
            <Row icon={Fingerprint} label="MAC" value={state?.mac} mono />
          </dl>
        </CardContent>
      </Card>

      <NameCard key={device?.id} id={device?.id} name={state?.name ?? ''} disabled={disabled} send={send} />

      <FirmwareCard
        installed={state?.fw}
        firmware={firmware}
        ota={device?.ota ?? null}
        disabled={disabled}
        update={update}
        dismissOta={dismissOta}
        now={now}
      />

      <Card>
        <CardHeader>
          <CardTitle className="flex items-center gap-2">
            <Timer className="size-4" /> Reporting
          </CardTitle>
          <CardDescription>
            How often the device sends its status when nothing changes. Changes are always sent immediately.
          </CardDescription>
        </CardHeader>
        <CardContent className="flex flex-wrap items-center gap-3">
          <Select
            value={state ? String(state.interval) : undefined}
            onValueChange={(v) => send({ interval: Number(v) })}
            disabled={disabled}
          >
            <SelectTrigger className="w-56">
              <SelectValue placeholder="—" />
            </SelectTrigger>
            <SelectContent>
              {intervals.map((i) => (
                <SelectItem key={i.s} value={String(i.s)}>
                  {i.label}
                </SelectItem>
              ))}
            </SelectContent>
          </Select>
          <Button variant="outline" onClick={() => send({ action: 'state' })} disabled={disabled}>
            <RefreshCw /> Refresh now
          </Button>
        </CardContent>
      </Card>

      <Card className="border-destructive/30">
        <CardHeader>
          <CardTitle className="flex items-center gap-2">
            <RotateCcw className="size-4" /> Restart
          </CardTitle>
          <CardDescription>
            Reboots the ESP32. It reconnects on its own in about 10 seconds and keeps all LED settings.
          </CardDescription>
        </CardHeader>
        <CardContent>
          <AlertDialog>
            <AlertDialogTrigger asChild>
              <Button variant="destructive" disabled={disabled}>
                <RotateCcw /> Restart device
              </Button>
            </AlertDialogTrigger>
            <AlertDialogContent>
              <AlertDialogHeader>
                <AlertDialogTitle>Restart the ESP32?</AlertDialogTitle>
                <AlertDialogDescription>
                  The device goes offline for a few seconds while it reboots and reconnects.
                </AlertDialogDescription>
              </AlertDialogHeader>
              <AlertDialogFooter>
                <AlertDialogCancel>Cancel</AlertDialogCancel>
                <AlertDialogAction variant="destructive" onClick={() => send({ action: 'restart' })}>
                  Restart
                </AlertDialogAction>
              </AlertDialogFooter>
            </AlertDialogContent>
          </AlertDialog>
        </CardContent>
      </Card>

      {device && device.status !== 'online' && (
        <Card>
          <CardHeader>
            <CardTitle className="flex items-center gap-2">
              <Trash2 className="size-4" /> Remove from list
            </CardTitle>
            <CardDescription>
              For a board that is gone for good. It clears what the broker remembers about it. If the board ever comes
              back online, it shows up again.
            </CardDescription>
          </CardHeader>
          <CardContent>
            <Button variant="outline" onClick={forget}>
              <Trash2 /> Forget {device.state?.name || device.id}
            </Button>
          </CardContent>
        </Card>
      )}
    </div>
  )
}

function NameCard({
  id,
  name,
  disabled,
  send,
}: {
  id?: string
  name: string
  disabled: boolean
  send: (cmd: Command) => void
}) {
  const [value, setValue] = useState(name)
  // Follow the reported name unless the user is mid-edit
  const [editing, setEditing] = useState(false)
  useEffect(() => {
    if (!editing) setValue(name)
  }, [name, editing])

  const changed = value.trim() !== name
  return (
    <Card>
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <Pencil className="size-4" /> Name
        </CardTitle>
        <CardDescription>Shown in the device list instead of the id {id && <span className="font-mono">{id}</span>}.</CardDescription>
      </CardHeader>
      <CardContent>
        <form
          className="flex gap-2"
          onSubmit={(e) => {
            e.preventDefault()
            send({ name: value.trim() })
            setEditing(false)
          }}
        >
          <Input
            value={value}
            maxLength={32}
            placeholder="e.g. Living room"
            onChange={(e) => {
              setValue(e.target.value)
              setEditing(true)
            }}
            disabled={disabled}
          />
          <Button type="submit" disabled={disabled || !changed}>
            Save
          </Button>
        </form>
      </CardContent>
    </Card>
  )
}

const OTA_LABELS: Record<OtaProgress['state'], string> = {
  starting: 'Asking the device to update…',
  downloading: 'Downloading and installing',
  rebooting: 'Rebooting into the new firmware…',
  failed: 'Update failed',
  done: 'Update complete',
}

function FirmwareCard({
  installed,
  firmware,
  ota,
  disabled,
  update,
  dismissOta,
  now,
}: {
  installed?: string
  firmware: ReturnType<typeof useFirmware>
  ota: OtaProgress | null
  disabled: boolean
  update: (url: string, version: string) => void
  dismissOta: () => void
  now: number
}) {
  const { latest, binUrl, checking, checkedAt, error, check } = firmware
  const cmp = latest && installed ? compareVersions(latest.version, installed) : null
  const busy = !!ota && ota.state !== 'failed' && ota.state !== 'done'

  let status: React.ReactNode
  if (ota) {
    status = (
      <div className="grid gap-2">
        <div
          className={cn(
            'flex items-center gap-2 text-sm',
            ota.state === 'failed' && 'text-red-600 dark:text-red-400',
            ota.state === 'done' && 'text-emerald-600 dark:text-emerald-400',
          )}
        >
          {busy ? (
            <Loader2 className="size-4 animate-spin" />
          ) : ota.state === 'done' ? (
            <CircleCheck className="size-4" />
          ) : (
            <CircleAlert className="size-4" />
          )}
          <span className="flex-1">
            {OTA_LABELS[ota.state]}
            {ota.state === 'downloading' && ota.progress !== undefined && ` (${ota.progress}%)`}
            {ota.error && `: ${ota.error}`}
          </span>
          {!busy && (
            <button className="text-xs text-muted-foreground underline" onClick={dismissOta}>
              Dismiss
            </button>
          )}
        </div>
        {busy && (
          <div className="h-2 overflow-hidden rounded-full bg-muted">
            <div
              className="h-full rounded-full bg-sky-500 transition-[width] duration-500"
              style={{ width: `${ota.state === 'rebooting' ? 100 : (ota.progress ?? 0)}%` }}
            />
          </div>
        )}
      </div>
    )
  } else if (error) {
    status = <p className="text-sm text-red-600 dark:text-red-400">Could not check for updates: {error}</p>
  } else if (!latest || cmp === null) {
    status = (
      <p className="text-sm text-muted-foreground">
        {checking ? 'Checking…' : 'Waiting for the device to report its version'}
      </p>
    )
  } else if (cmp > 0) {
    status = (
      <p className="flex items-center gap-2 text-sm text-sky-600 dark:text-sky-400">
        <Download className="size-4" /> v{latest.version} is available
      </p>
    )
  } else {
    status = (
      <p className="flex items-center gap-2 text-sm text-emerald-600 dark:text-emerald-400">
        <CircleCheck className="size-4" />
        {cmp === 0 ? 'Up to date' : `The device runs a newer build than the published v${latest.version}`}
      </p>
    )
  }

  return (
    <Card>
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <Package className="size-4" /> Firmware
        </CardTitle>
        <CardDescription>
          Installed {installed ? `v${installed}` : '—'} · Latest published {latest ? `v${latest.version}` : '—'}
          {latest && ` (built ${formatDuration((now - Date.parse(latest.built)) / 1000)} ago)`}
        </CardDescription>
      </CardHeader>
      <CardContent className="grid gap-4">
        {status}
        <div className="flex flex-wrap items-center gap-3">
          <Button variant="outline" onClick={() => void check()} disabled={checking}>
            <RefreshCw className={cn(checking && 'animate-spin')} /> Check for updates
          </Button>
          {latest && cmp !== null && cmp > 0 && binUrl && (
            <AlertDialog>
              <AlertDialogTrigger asChild>
                <Button disabled={disabled || busy}>
                  <Download /> Update to v{latest.version}
                </Button>
              </AlertDialogTrigger>
              <AlertDialogContent>
                <AlertDialogHeader>
                  <AlertDialogTitle>Install firmware v{latest.version}?</AlertDialogTitle>
                  <AlertDialogDescription>
                    The ESP32 downloads the new firmware ({formatBytes(latest.size)}), installs it and reboots. This
                    takes about a minute. If the new version can't reconnect to the broker, the device goes back to
                    v{installed} by itself.
                  </AlertDialogDescription>
                </AlertDialogHeader>
                <AlertDialogFooter>
                  <AlertDialogCancel>Cancel</AlertDialogCancel>
                  <AlertDialogAction onClick={() => update(binUrl, latest.version)}>Update</AlertDialogAction>
                </AlertDialogFooter>
              </AlertDialogContent>
            </AlertDialog>
          )}
          {checkedAt && (
            <span className="text-xs text-muted-foreground">Checked {formatDuration((now - checkedAt) / 1000)} ago</span>
          )}
        </div>
      </CardContent>
    </Card>
  )
}

function Stat({
  icon: Icon,
  label,
  value,
  detail,
  extra,
}: {
  icon: typeof Clock
  label: string
  value: string
  detail?: string
  extra?: React.ReactNode
}) {
  return (
    <Card className="gap-1 py-4">
      <CardContent className="grid gap-1 px-4">
        <div className="flex items-center justify-between text-xs text-muted-foreground">
          <span className="flex items-center gap-1.5">
            <Icon className="size-3.5" /> {label}
          </span>
          {extra}
        </div>
        <div className="text-xl font-semibold tabular-nums">{value}</div>
        <div className="h-4 text-xs text-muted-foreground">{detail}</div>
      </CardContent>
    </Card>
  )
}

function Row({ icon: Icon, label, value, mono }: { icon: typeof Clock; label: string; value?: string; mono?: boolean }) {
  return (
    <div className="flex items-center justify-between gap-4">
      <dt className="flex items-center gap-2 text-muted-foreground">
        <Icon className="size-4" /> {label}
      </dt>
      <dd className={cn('truncate text-right', mono && 'font-mono text-xs')}>{value || '—'}</dd>
    </div>
  )
}

function SignalBars({ bars }: { bars: number }) {
  return (
    <span className="flex items-end gap-0.5" aria-hidden>
      {[1, 2, 3, 4].map((b) => (
        <span
          key={b}
          className={cn('w-1 rounded-sm', b <= bars ? 'bg-emerald-500' : 'bg-muted-foreground/25')}
          style={{ height: 3 + b * 3 }}
        />
      ))}
    </span>
  )
}

function useNow(intervalMs = 1000) {
  const [now, setNow] = useState(Date.now())
  useEffect(() => {
    const id = setInterval(() => setNow(Date.now()), intervalMs)
    return () => clearInterval(id)
  }, [intervalMs])
  return now
}
