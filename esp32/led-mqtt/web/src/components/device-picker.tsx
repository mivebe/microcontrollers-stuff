import { Download, Loader2, X } from 'lucide-react'
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
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select'
import type { UpdateQueue } from '@/hooks/use-devices'
import type { useFirmware } from '@/hooks/use-firmware'
import { cn } from '@/lib/utils'
import { type Device, compareVersions, deviceLabel } from '@/lib/device'

interface Props {
  devices: Device[]
  selectedId: string | null
  select: (id: string) => void
  firmware: ReturnType<typeof useFirmware>
  queue: UpdateQueue | null
  updateAll: (url: string, version: string) => void
  cancelUpdateAll: () => void
}

export function DevicePicker({ devices, selectedId, select, firmware, queue, updateAll, cancelUpdateAll }: Props) {
  const { latest, binUrl } = firmware
  const behind = (d: Device) => !!latest && !!d.state && compareVersions(latest.version, d.state.fw) > 0
  const updatable = devices.filter((d) => d.status === 'online' && behind(d))

  return (
    <div className="flex flex-wrap items-center gap-2">
      <Select value={selectedId && devices.some((d) => d.id === selectedId) ? selectedId : undefined} onValueChange={select}>
        <SelectTrigger className="min-w-0 flex-1 sm:max-w-80" aria-label="Device">
          <SelectValue placeholder={devices.length ? 'Choose a device' : 'No devices found yet'} />
        </SelectTrigger>
        <SelectContent>
          {devices.map((d) => (
            <SelectItem key={d.id} value={d.id}>
              <span
                className={cn('size-2 shrink-0 rounded-full', d.status === 'online' ? 'bg-emerald-500' : 'bg-red-500')}
                aria-label={d.status}
              />
              <span className="truncate">{deviceLabel(d)}</span>
              {d.state?.name && <span className="font-mono text-xs text-muted-foreground">{d.id}</span>}
              {d.state?.fw && <span className="text-xs text-muted-foreground">v{d.state.fw}</span>}
              {behind(d) && <Download className="size-3.5 text-sky-500" aria-label="Update available" />}
            </SelectItem>
          ))}
        </SelectContent>
      </Select>

      {queue ? (
        <div className="flex items-center gap-2 text-sm text-muted-foreground">
          <Loader2 className="size-4 animate-spin text-sky-500" />
          Updating {queue.done + 1} of {queue.total}
          <Button variant="ghost" size="icon" onClick={cancelUpdateAll} aria-label="Stop after the current board">
            <X />
          </Button>
        </div>
      ) : (
        latest &&
        binUrl &&
        updatable.length > 1 && (
          <AlertDialog>
            <AlertDialogTrigger asChild>
              <Button variant="outline">
                <Download /> Update all ({updatable.length})
              </Button>
            </AlertDialogTrigger>
            <AlertDialogContent>
              <AlertDialogHeader>
                <AlertDialogTitle>Update {updatable.length} boards to v{latest.version}?</AlertDialogTitle>
                <AlertDialogDescription>
                  The boards update one at a time: {updatable.map(deviceLabel).join(', ')}. If one fails, the rest are
                  left alone. Offline boards are skipped.
                </AlertDialogDescription>
              </AlertDialogHeader>
              <AlertDialogFooter>
                <AlertDialogCancel>Cancel</AlertDialogCancel>
                <AlertDialogAction onClick={() => updateAll(binUrl, latest.version)}>Update all</AlertDialogAction>
              </AlertDialogFooter>
            </AlertDialogContent>
          </AlertDialog>
        )
      )}
    </div>
  )
}
