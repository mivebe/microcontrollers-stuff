import { useState } from 'react'
import { ThemeProvider, useTheme } from 'next-themes'
import {
  Cloud,
  CloudOff,
  Cpu,
  Download,
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
import { DevicePicker } from '@/components/device-picker'
import { DeviceTab } from '@/components/device-tab'
import { LogsTab } from '@/components/logs-tab'
import { useDevices, type BrokerStatus } from '@/hooks/use-devices'
import { useFirmware } from '@/hooks/use-firmware'
import { cn } from '@/lib/utils'
import { type ConnectionSettings, type DeviceStatus, DEFAULT_SETTINGS, compareVersions, deviceLabel } from '@/lib/device'

const STORAGE_KEY = 'led-mqtt-settings'

function loadSettings(): ConnectionSettings | null {
  try {
    const saved = JSON.parse(localStorage.getItem(STORAGE_KEY) ?? 'null')
    if (!saved?.username) return null
    // Older versions stored one device's full prefix ("mivebe/esp32-1"); keep its base
    if (!saved.base && typeof saved.prefix === 'string') saved.base = saved.prefix.split('/')[0]
    const { prefix: _prefix, ...rest } = saved
    return { ...DEFAULT_SETTINGS, ...rest }
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
  const devices = useDevices(settings)
  const device = devices.selected
  const firmware = useFirmware()
  const [tab, setTab] = useState('control')

  const connected = devices.broker === 'connected'
  const controllable = connected && device?.status === 'online'
  const latestFw = firmware.latest?.version
  const updateAvailable = !!latestFw && !!device?.state?.fw && compareVersions(latestFw, device.state.fw) > 0

  function save(s: ConnectionSettings | null) {
    storeSettings(s)
    setSettings(s)
    setSettingsOpen(false)
  }

  return (
    <div className="mx-auto flex min-h-svh max-w-2xl flex-col gap-4 px-4 pb-10">
      <header className="sticky top-0 z-40 -mx-4 flex items-center gap-3 border-b bg-background/80 px-4 pt-[max(0.75rem,env(safe-area-inset-top))] pb-3 backdrop-blur-md">
        <div className="grid size-10 place-items-center rounded-xl bg-sky-500 text-white shadow-lg shadow-sky-500/30">
          <Cpu className="size-5" />
        </div>
        <div className="min-w-0 flex-1">
          <h1 className="text-lg leading-tight font-semibold">ESP32 Remote</h1>
          <p className="truncate text-xs text-muted-foreground">
            {!settings
              ? 'not connected'
              : `${devices.devices.length} device${devices.devices.length === 1 ? '' : 's'} · ${settings.base}`}
          </p>
        </div>
        {/* Fixed-size slot so the spinner never shifts the layout */}
        <span className="grid size-8 place-items-center" role="status" aria-live="polite">
          {devices.pending && <Loader2 className="size-4 animate-spin text-sky-500" aria-label="Applying change" />}
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
          <DevicePicker
            devices={devices.devices}
            selectedId={devices.selectedId}
            select={devices.select}
            firmware={firmware}
            queue={devices.queue}
            updateAll={devices.updateAll}
            cancelUpdateAll={devices.cancelUpdateAll}
          />

          <div className="flex flex-wrap gap-2">
            <BrokerBadge status={devices.broker} />
            {device && <DeviceBadge status={device.status} />}
            {updateAvailable && (
              <button onClick={() => setTab('device')}>
                <Badge variant="outline" className="gap-1.5 text-sky-600 dark:text-sky-400">
                  <Download /> Update v{latestFw} available
                </Badge>
              </button>
            )}
          </div>

          {devices.error && devices.broker !== 'connected' && (
            <Banner tone="error">
              Broker: {devices.error}.{' '}
              <button className="underline" onClick={() => setSettingsOpen(true)}>
                Check settings
              </button>
            </Banner>
          )}
          {connected && device?.status === 'offline' && (
            <Banner tone="warn">
              {deviceLabel(device)} is offline (no power or no WiFi). Controls unlock as soon as it reconnects.
            </Banner>
          )}
          {connected && !devices.devices.length && (
            <Banner tone="warn">
              No boards found under <span className="font-mono">{settings.base}/</span> yet. They appear here as soon as
              they connect to the broker.
            </Banner>
          )}

          <Tabs value={tab} onValueChange={setTab} className="gap-4">
            <TabsList className="w-full">
              <TabsTrigger value="control">
                <SlidersHorizontal /> Control
              </TabsTrigger>
              <TabsTrigger value="logs">
                <ScrollText /> Logs
                {devices.logs.some((l) => l.lvl === 'E') && <span className="size-1.5 rounded-full bg-red-500" />}
              </TabsTrigger>
              <TabsTrigger value="device">
                <Cpu /> Device
                {updateAvailable && <span className="size-1.5 rounded-full bg-sky-500" />}
              </TabsTrigger>
            </TabsList>
            <TabsContent value="control">
              <ControlTab state={device?.state ?? null} disabled={!controllable} send={devices.send} />
            </TabsContent>
            <TabsContent value="logs">
              <LogsTab logs={devices.logs} connected={controllable} send={devices.send} clear={devices.clearLogs} />
            </TabsContent>
            <TabsContent value="device">
              <DeviceTab
                key={device?.id}
                device={device}
                disabled={!controllable}
                send={devices.send}
                firmware={firmware}
                update={(url, version) => device && devices.update(device.id, url, version)}
                dismissOta={() => device && devices.dismissOta(device.id)}
                forget={() => device && devices.forget(device.id)}
              />
            </TabsContent>
          </Tabs>

          <Dialog open={settingsOpen} onOpenChange={setSettingsOpen}>
            <DialogContent>
              <DialogHeader>
                <DialogTitle>Connection settings</DialogTitle>
                <DialogDescription>HiveMQ Cloud login and the topic base your boards use.</DialogDescription>
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
