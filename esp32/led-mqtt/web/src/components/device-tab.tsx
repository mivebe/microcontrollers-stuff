import { useEffect, useState } from 'react'
import {
  Clock,
  Cpu,
  Fingerprint,
  MemoryStick,
  Network,
  Package,
  RefreshCw,
  RotateCcw,
  Timer,
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
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import { cn } from '@/lib/utils'
import { type Command, type DeviceState, formatBytes, formatDuration, signalQuality } from '@/lib/device'

const INTERVALS = [
  { s: 10, label: 'Every 10 seconds' },
  { s: 30, label: 'Every 30 seconds' },
  { s: 60, label: 'Every minute' },
  { s: 300, label: 'Every 5 minutes' },
  { s: 900, label: 'Every 15 minutes' },
]

interface Props {
  state: DeviceState | null
  stateAt: number | null
  disabled: boolean
  send: (cmd: Command) => void
}

export function DeviceTab({ state, stateAt, disabled, send }: Props) {
  const now = useNow()
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
            <Row icon={Package} label="Firmware" value={state?.fw && `v${state.fw}`} />
            <Row icon={Cpu} label="Chip" value={state?.chip && `${state.chip} @ ${state.cpuMhz} MHz`} />
            <Row icon={Wifi} label="Network" value={state?.ssid} />
            <Row icon={Network} label="Local IP" value={state?.ip} mono />
            <Row icon={Fingerprint} label="MAC" value={state?.mac} mono />
          </dl>
        </CardContent>
      </Card>

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
    </div>
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
