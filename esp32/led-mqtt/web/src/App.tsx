import { useState } from 'react'
import { ThemeProvider, useTheme } from 'next-themes'
import {
  Cloud,
  CloudOff,
  Cpu,
  Loader2,
  Moon,
  ScrollText,
  Settings,
  SlidersHorizontal,
  Sun,
  Wifi,
  WifiOff,
} from 'lucide-react'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import { Dialog, DialogContent, DialogDescription, DialogHeader, DialogTitle } from '@/components/ui/dialog'
import { Toaster } from '@/components/ui/sonner'
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs'
import { TooltipProvider } from '@/components/ui/tooltip'
import { ConnectionForm } from '@/components/connection-form'
import { ControlTab } from '@/components/control-tab'
import { DeviceTab } from '@/components/device-tab'
import { LogsTab } from '@/components/logs-tab'
import { useDevice, type BrokerStatus, type DeviceStatus } from '@/hooks/use-device'
import { cn } from '@/lib/utils'
import { type ConnectionSettings, DEFAULT_SETTINGS } from '@/lib/device'

const STORAGE_KEY = 'led-mqtt-settings'

function loadSettings(): ConnectionSettings | null {
  try {
    const saved = JSON.parse(localStorage.getItem(STORAGE_KEY) ?? 'null')
    return saved?.username ? { ...DEFAULT_SETTINGS, ...saved } : null
  } catch {
    return null
  }
}

function storeSettings(s: ConnectionSettings | null) {
  try {
    if (s) localStorage.setItem(STORAGE_KEY, JSON.stringify(s))
    else localStorage.removeItem(STORAGE_KEY)
  } catch {
    // private mode etc. - settings just won't persist
  }
}

export default function App() {
  return (
    <ThemeProvider attribute="class" defaultTheme="system" enableSystem disableTransitionOnChange>
      <TooltipProvider>
        <Dashboard />
        <Toaster position="top-center" />
      </TooltipProvider>
    </ThemeProvider>
  )
}

function Dashboard() {
  const [settings, setSettings] = useState<ConnectionSettings | null>(loadSettings)
  const [settingsOpen, setSettingsOpen] = useState(false)
  const device = useDevice(settings)

  const connected = device.broker === 'connected'
  const controllable = connected && device.status === 'online'

  function save(s: ConnectionSettings | null) {
    storeSettings(s)
    setSettings(s)
    setSettingsOpen(false)
  }

  return (
    <div className="mx-auto flex min-h-svh max-w-2xl flex-col gap-4 px-4 pt-4 pb-10">
      <header className="flex items-center gap-3">
        <div className="grid size-10 place-items-center rounded-xl bg-sky-500 text-white shadow-lg shadow-sky-500/30">
          <Cpu className="size-5" />
        </div>
        <div className="min-w-0 flex-1">
          <h1 className="text-lg leading-tight font-semibold">ESP32 Remote</h1>
          <p className="truncate font-mono text-xs text-muted-foreground">{settings?.prefix ?? 'not connected'}</p>
        </div>
        {/* Fixed-size slot so the spinner never shifts the layout */}
        <span className="grid size-8 place-items-center" role="status" aria-live="polite">
          {device.pending && <Loader2 className="size-4 animate-spin text-sky-500" aria-label="Applying change" />}
        </span>
        <ThemeToggle />
        {settings && (
          <Button variant="outline" size="icon" onClick={() => setSettingsOpen(true)} aria-label="Connection settings">
            <Settings />
          </Button>
        )}
      </header>

      {!settings ? (
        <Card>
          <CardHeader>
            <CardTitle>Connect to your broker</CardTitle>
            <CardDescription>Log in to HiveMQ Cloud to reach the ESP32 from anywhere.</CardDescription>
          </CardHeader>
          <CardContent>
            <ConnectionForm initial={DEFAULT_SETTINGS} onSave={save} />
          </CardContent>
        </Card>
      ) : (
        <>
          <div className="flex flex-wrap gap-2">
            <BrokerBadge status={device.broker} />
            <DeviceBadge status={device.status} />
          </div>

          {device.error && device.broker !== 'connected' && (
            <Banner tone="error">
              Broker: {device.error}.{' '}
              <button className="underline" onClick={() => setSettingsOpen(true)}>
                Check settings
              </button>
            </Banner>
          )}
          {connected && device.status === 'offline' && (
            <Banner tone="warn">
              The ESP32 is offline (no power or no WiFi). Controls unlock as soon as it reconnects.
            </Banner>
          )}

          <Tabs defaultValue="control" className="gap-4">
            <TabsList className="w-full">
              <TabsTrigger value="control">
                <SlidersHorizontal /> Control
              </TabsTrigger>
              <TabsTrigger value="logs">
                <ScrollText /> Logs
                {device.logs.some((l) => l.lvl === 'E') && <span className="size-1.5 rounded-full bg-red-500" />}
              </TabsTrigger>
              <TabsTrigger value="device">
                <Cpu /> Device
              </TabsTrigger>
            </TabsList>
            <TabsContent value="control">
              <ControlTab state={device.state} disabled={!controllable} send={device.send} />
            </TabsContent>
            <TabsContent value="logs">
              <LogsTab logs={device.logs} connected={controllable} send={device.send} clear={device.clearLogs} />
            </TabsContent>
            <TabsContent value="device">
              <DeviceTab state={device.state} stateAt={device.stateAt} disabled={!controllable} send={device.send} />
            </TabsContent>
          </Tabs>

          <Dialog open={settingsOpen} onOpenChange={setSettingsOpen}>
            <DialogContent>
              <DialogHeader>
                <DialogTitle>Connection settings</DialogTitle>
                <DialogDescription>HiveMQ Cloud login and the device's topic prefix.</DialogDescription>
              </DialogHeader>
              <ConnectionForm initial={settings} onSave={save} onForget={() => save(null)} submitLabel="Save & reconnect" />
            </DialogContent>
          </Dialog>
        </>
      )}
    </div>
  )
}

function BrokerBadge({ status }: { status: BrokerStatus }) {
  const ok = status === 'connected'
  return (
    <Badge variant="outline" className={cn('gap-1.5', ok ? 'text-emerald-600 dark:text-emerald-400' : 'text-amber-600 dark:text-amber-400')}>
      {status === 'connecting' ? <Loader2 className="animate-spin" /> : ok ? <Cloud /> : <CloudOff />}
      Broker {status}
    </Badge>
  )
}

function DeviceBadge({ status }: { status: DeviceStatus }) {
  const online = status === 'online'
  return (
    <Badge
      variant="outline"
      className={cn('gap-1.5', online ? 'text-emerald-600 dark:text-emerald-400' : 'text-red-600 dark:text-red-400')}
    >
      <span className={cn('size-1.5 rounded-full', online ? 'animate-pulse bg-emerald-500' : 'bg-red-500')} />
      {online ? <Wifi /> : <WifiOff />}
      Device {status}
    </Badge>
  )
}

function Banner({ tone, children }: { tone: 'warn' | 'error'; children: React.ReactNode }) {
  return (
    <div
      className={cn(
        'rounded-lg border px-3 py-2 text-sm',
        tone === 'warn'
          ? 'border-amber-500/30 bg-amber-500/10 text-amber-700 dark:text-amber-300'
          : 'border-red-500/30 bg-red-500/10 text-red-700 dark:text-red-300',
      )}
    >
      {children}
    </div>
  )
}

function ThemeToggle() {
  const { resolvedTheme, setTheme } = useTheme()
  const dark = resolvedTheme === 'dark'
  return (
    <Button variant="ghost" size="icon" onClick={() => setTheme(dark ? 'light' : 'dark')} aria-label="Toggle theme">
      {dark ? <Sun /> : <Moon />}
    </Button>
  )
}
