import { useEffect, useMemo, useRef, useState } from 'react'
import {
  ArrowDownToLine,
  CircleAlert,
  Download,
  History,
  Info,
  Pause,
  Play,
  ScrollText,
  Search,
  Trash2,
  TriangleAlert,
} from 'lucide-react'
import { Button } from '@/components/ui/button'
import { Card, CardContent } from '@/components/ui/card'
import { Input } from '@/components/ui/input'
import { ToggleGroup, ToggleGroupItem } from '@/components/ui/toggle-group'
import { Tooltip, TooltipContent, TooltipTrigger } from '@/components/ui/tooltip'
import { cn } from '@/lib/utils'
import type { Command, LogEntry, LogLevel } from '@/lib/device'

type Filter = 'all' | LogLevel

const LEVELS: Record<LogLevel, { label: string; icon: typeof Info; className: string }> = {
  I: { label: 'Info', icon: Info, className: 'text-sky-600 dark:text-sky-400' },
  W: { label: 'Warn', icon: TriangleAlert, className: 'text-amber-600 dark:text-amber-400' },
  E: { label: 'Error', icon: CircleAlert, className: 'text-red-600 dark:text-red-400' },
}

interface Props {
  logs: LogEntry[]
  connected: boolean
  send: (cmd: Command) => void
  clear: () => void
}

export function LogsTab({ logs, connected, send, clear }: Props) {
  const [filter, setFilter] = useState<Filter>('all')
  const [query, setQuery] = useState('')
  const [follow, setFollow] = useState(true)
  const scrollRef = useRef<HTMLDivElement>(null)

  const counts = useMemo(() => {
    const c = { I: 0, W: 0, E: 0 }
    for (const l of logs) c[l.lvl]++
    return c
  }, [logs])

  const visible = useMemo(() => {
    const q = query.trim().toLowerCase()
    return logs.filter((l) => (filter === 'all' || l.lvl === filter) && (!q || l.msg.toLowerCase().includes(q)))
  }, [logs, filter, query])

  // Auto-scroll to the newest line while following
  useEffect(() => {
    const el = scrollRef.current
    if (follow && el) el.scrollTop = el.scrollHeight
  }, [visible, follow])

  function onScroll() {
    const el = scrollRef.current
    if (!el) return
    const atBottom = el.scrollHeight - el.scrollTop - el.clientHeight < 24
    if (atBottom !== follow) setFollow(atBottom)
  }

  function download() {
    const text = logs.map((l) => `${formatTime(l)}  ${LEVELS[l.lvl].label.toUpperCase().padEnd(5)}  ${l.msg}`).join('\n')
    const url = URL.createObjectURL(new Blob([text], { type: 'text/plain' }))
    const a = document.createElement('a')
    a.href = url
    a.download = `esp32-log-${new Date().toISOString().slice(0, 19).replace(/[:T]/g, '-')}.txt`
    a.click()
    URL.revokeObjectURL(url)
  }

  return (
    <Card className="gap-0 py-0">
      {/* Toolbar */}
      <div className="flex flex-col gap-3 border-b p-3">
        <div className="flex items-center gap-2">
          <div className="relative flex-1">
            <Search className="absolute top-1/2 left-2.5 size-4 -translate-y-1/2 text-muted-foreground" />
            <Input value={query} onChange={(e) => setQuery(e.target.value)} placeholder="Search logs…" className="pl-8" />
          </div>
          <IconButton label="Load history from device" onClick={() => send({ action: 'logs' })} disabled={!connected}>
            <History />
          </IconButton>
          <IconButton label={follow ? 'Pause auto-scroll' : 'Follow new lines'} onClick={() => setFollow(!follow)}>
            {follow ? <Pause /> : <Play />}
          </IconButton>
          <IconButton label="Download as text" onClick={download} disabled={!logs.length}>
            <Download />
          </IconButton>
          <IconButton label="Clear (only in this browser)" onClick={clear} disabled={!logs.length}>
            <Trash2 />
          </IconButton>
        </div>
        <ToggleGroup
          type="single"
          size="sm"
          variant="outline"
          value={filter}
          onValueChange={(v) => v && setFilter(v as Filter)}
          className="justify-start"
        >
          <ToggleGroupItem value="all">All {logs.length}</ToggleGroupItem>
          {(Object.keys(LEVELS) as LogLevel[]).map((lvl) => {
            const { label, icon: Icon, className } = LEVELS[lvl]
            return (
              <ToggleGroupItem key={lvl} value={lvl}>
                <Icon className={className} /> {label} {counts[lvl]}
              </ToggleGroupItem>
            )
          })}
        </ToggleGroup>
      </div>

      {/* Console */}
      <CardContent className="relative p-0">
        <div ref={scrollRef} onScroll={onScroll} className="h-[60vh] min-h-72 overflow-y-auto font-mono text-xs">
          {visible.length === 0 ? (
            <div className="flex h-full flex-col items-center justify-center gap-2 text-muted-foreground">
              <ScrollText className="size-8 opacity-50" />
              <p className="font-sans text-sm">{logs.length ? 'No lines match the filter' : 'No log lines yet'}</p>
            </div>
          ) : (
            <ul className="divide-y divide-border/50">
              {visible.map((l) => {
                const { icon: Icon, className } = LEVELS[l.lvl]
                return (
                  <li
                    key={l.key}
                    className={cn(
                      'flex items-start gap-2 px-3 py-1.5 hover:bg-muted/50',
                      l.lvl === 'E' && 'bg-red-500/5',
                      l.lvl === 'W' && 'bg-amber-500/5',
                    )}
                  >
                    <span className="shrink-0 text-muted-foreground tabular-nums">{formatTime(l)}</span>
                    <Icon className={cn('mt-px size-3.5 shrink-0', className)} />
                    <span className="min-w-0 break-words">{l.msg}</span>
                  </li>
                )
              })}
            </ul>
          )}
        </div>
        {!follow && visible.length > 0 && (
          <Button size="sm" variant="secondary" className="absolute right-3 bottom-3 shadow-md" onClick={() => setFollow(true)}>
            <ArrowDownToLine /> Latest
          </Button>
        )}
      </CardContent>
    </Card>
  )
}

function IconButton({
  label,
  children,
  ...props
}: { label: string; children: React.ReactNode } & React.ComponentProps<typeof Button>) {
  return (
    <Tooltip>
      <TooltipTrigger asChild>
        <Button variant="outline" size="icon" aria-label={label} {...props}>
          {children}
        </Button>
      </TooltipTrigger>
      <TooltipContent>{label}</TooltipContent>
    </Tooltip>
  )
}

function formatTime(l: LogEntry) {
  if (l.at === null) return `+${(l.up / 1000).toFixed(1)}s`
  const d = new Date(l.at)
  const today = new Date().toDateString() === d.toDateString()
  const time = d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false })
  return today ? time : `${d.toLocaleDateString([], { month: 'short', day: 'numeric' })} ${time}`
}
