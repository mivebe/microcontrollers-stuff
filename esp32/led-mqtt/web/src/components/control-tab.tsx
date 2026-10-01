import { useEffect, useState } from 'react'
import { Activity, Circle, Gauge, Lightbulb, LightbulbOff, Power, Sun, SunDim, Zap } from 'lucide-react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Slider } from '@/components/ui/slider'
import { Switch } from '@/components/ui/switch'
import { ToggleGroup, ToggleGroupItem } from '@/components/ui/toggle-group'
import { cn } from '@/lib/utils'
import type { Command, DeviceState, Mode } from '@/lib/device'

const MODES: { value: Mode; label: string; icon: typeof Circle; hint: string }[] = [
  { value: 'solid', label: 'Solid', icon: Circle, hint: 'Steady light' },
  { value: 'blink', label: 'Blink', icon: Zap, hint: 'On / off flashing' },
  { value: 'breathe', label: 'Breathe', icon: Activity, hint: 'Smooth fade in and out' },
]

const SPEED_PRESETS = [
  { label: 'Fast', ms: 300 },
  { label: 'Normal', ms: 1000 },
  { label: 'Slow', ms: 3000 },
]

interface Props {
  state: DeviceState | null
  disabled: boolean
  send: (cmd: Command) => void
}

export function ControlTab({ state, disabled, send }: Props) {
  const on = state?.on ?? false
  const mode = state?.mode ?? 'solid'

  // Sliders keep a local value while dragging and only send when released
  const [brightness, setBrightness] = useState(state?.brightness ?? 100)
  const [period, setPeriod] = useState(state?.period ?? 1000)
  useEffect(() => setBrightness(state?.brightness ?? 100), [state?.brightness])
  useEffect(() => setPeriod(state?.period ?? 1000), [state?.period])

  const modeInfo = MODES.find((m) => m.value === mode) ?? MODES[0]

  return (
    <div className="grid gap-4">
      {/* Power */}
      <Card className="overflow-hidden">
        <CardContent className="flex flex-col items-center gap-5 py-6">
          <button
            type="button"
            disabled={disabled}
            onClick={() => send({ on: !on })}
            aria-label={on ? 'Turn LED off' : 'Turn LED on'}
            className={cn(
              'relative grid size-36 place-items-center rounded-full border-2 transition-all duration-300',
              'focus-visible:ring-4 focus-visible:ring-sky-500/40 focus-visible:outline-none disabled:cursor-not-allowed disabled:opacity-50',
              on
                ? 'border-sky-400 bg-sky-500 text-white shadow-[0_0_60px_-5px] shadow-sky-500/70'
                : 'border-border bg-muted text-muted-foreground hover:border-foreground/30',
            )}
            style={on ? { opacity: disabled ? 0.5 : 0.55 + (brightness / 100) * 0.45 } : undefined}
          >
            {on && mode !== 'solid' && (
              <span
                className={cn('absolute inset-0 rounded-full bg-sky-400/40', mode === 'blink' ? 'animate-ping' : 'animate-pulse')}
                style={{ animationDuration: `${period}ms` }}
              />
            )}
            {on ? <Lightbulb className="relative size-14" /> : <LightbulbOff className="relative size-14" />}
          </button>

          <div className="flex items-center gap-3">
            <Power className="size-4 text-muted-foreground" />
            <span className="text-sm font-medium">{on ? 'On' : 'Off'}</span>
            <Switch checked={on} disabled={disabled} onCheckedChange={(v) => send({ on: v })} aria-label="LED power" />
          </div>
          <p className="text-sm text-muted-foreground">
            {state ? (
              <>
                {modeInfo.label} · {state.brightness}%{mode !== 'solid' && ` · ${(state.period / 1000).toFixed(1)} s cycle`}
              </>
            ) : (
              'Waiting for the device…'
            )}
          </p>
        </CardContent>
      </Card>

      {/* Brightness */}
      <Card>
        <CardHeader>
          <CardTitle className="flex items-center gap-2">
            <Sun className="size-4" /> Brightness
          </CardTitle>
          <CardDescription>How bright the LED shines when on.</CardDescription>
        </CardHeader>
        <CardContent className="flex items-center gap-4">
          <SunDim className="size-4 shrink-0 text-muted-foreground" />
          <Slider
            value={[brightness]}
            min={1}
            max={100}
            step={1}
            disabled={disabled}
            onValueChange={([v]) => setBrightness(v)}
            onValueCommit={([v]) => send({ brightness: v })}
            aria-label="Brightness"
            className="[&_[data-slot=slider-range]]:bg-sky-500"
          />
          <Sun className="size-5 shrink-0 text-muted-foreground" />
          <span className="w-12 text-right font-mono text-sm tabular-nums">{brightness}%</span>
        </CardContent>
      </Card>

      {/* Mode */}
      <Card>
        <CardHeader>
          <CardTitle className="flex items-center gap-2">
            <Activity className="size-4" /> Effect
          </CardTitle>
          <CardDescription>{modeInfo.hint}</CardDescription>
        </CardHeader>
        <CardContent>
          <ToggleGroup
            type="single"
            variant="outline"
            value={mode}
            disabled={disabled}
            onValueChange={(v) => v && send({ mode: v as Mode })}
            className="grid w-full grid-cols-3"
          >
            {MODES.map(({ value, label, icon: Icon }) => (
              <ToggleGroupItem key={value} value={value} className="h-16 flex-col gap-1.5 data-[state=on]:border-sky-500 data-[state=on]:bg-sky-500/10 data-[state=on]:text-sky-600 dark:data-[state=on]:text-sky-400" aria-label={label}>
                <Icon className="size-5" />
                <span className="text-xs">{label}</span>
              </ToggleGroupItem>
            ))}
          </ToggleGroup>
        </CardContent>
      </Card>

      {/* Speed */}
      <Card className={cn(mode === 'solid' && 'opacity-60')}>
        <CardHeader>
          <CardTitle className="flex items-center gap-2">
            <Gauge className="size-4" /> Speed
          </CardTitle>
          <CardDescription>
            {mode === 'solid' ? 'Only used by Blink and Breathe.' : 'Length of one blink / breath cycle.'}
          </CardDescription>
        </CardHeader>
        <CardContent className="grid gap-4">
          <div className="flex items-center gap-4">
            <Slider
              value={[period]}
              min={100}
              max={5000}
              step={100}
              disabled={disabled || mode === 'solid'}
              onValueChange={([v]) => setPeriod(v)}
              onValueCommit={([v]) => send({ period: v })}
              aria-label="Cycle length"
              className="[&_[data-slot=slider-range]]:bg-sky-500"
            />
            <span className="w-14 text-right font-mono text-sm tabular-nums">{(period / 1000).toFixed(1)} s</span>
          </div>
          <div className="grid grid-cols-3 gap-2">
            {SPEED_PRESETS.map((p) => (
              <button
                key={p.ms}
                type="button"
                disabled={disabled || mode === 'solid'}
                onClick={() => send({ period: p.ms })}
                className={cn(
                  'rounded-md border px-3 py-1.5 text-xs transition-colors hover:bg-muted disabled:pointer-events-none disabled:opacity-50',
                  state?.period === p.ms && 'border-sky-500 bg-sky-500/10 font-medium text-sky-600 dark:text-sky-400',
                )}
              >
                {p.label}
              </button>
            ))}
          </div>
        </CardContent>
      </Card>
    </div>
  )
}
