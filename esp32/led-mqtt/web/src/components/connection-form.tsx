import { useState } from 'react'
import { Eye, EyeOff, KeyRound, Link2, Tag, User } from 'lucide-react'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import type { ConnectionSettings } from '@/lib/device'

interface Props {
  initial: ConnectionSettings
  onSave: (s: ConnectionSettings) => void
  onForget?: () => void
  submitLabel?: string
}

export function ConnectionForm({ initial, onSave, onForget, submitLabel = 'Connect' }: Props) {
  const [form, setForm] = useState(initial)
  const [showPassword, setShowPassword] = useState(false)
  const set = (key: keyof ConnectionSettings) => (e: React.ChangeEvent<HTMLInputElement>) =>
    setForm({ ...form, [key]: e.target.value })

  return (
    <form
      className="grid gap-4"
      onSubmit={(e) => {
        e.preventDefault()
        onSave({ ...form, url: form.url.trim(), username: form.username.trim(), base: form.base.trim().replace(/\/+$/, '') })
      }}
    >
      <Field id="username" label="Username" icon={User}>
        <Input id="username" value={form.username} onChange={set('username')} autoComplete="username" required />
      </Field>
      <Field id="password" label="Password" icon={KeyRound}>
        <div className="relative">
          <Input
            id="password"
            type={showPassword ? 'text' : 'password'}
            value={form.password}
            onChange={set('password')}
            autoComplete="current-password"
            className="pr-9"
            required
          />
          <button
            type="button"
            onClick={() => setShowPassword(!showPassword)}
            className="absolute top-1/2 right-2.5 -translate-y-1/2 text-muted-foreground hover:text-foreground"
            aria-label={showPassword ? 'Hide password' : 'Show password'}
          >
            {showPassword ? <EyeOff className="size-4" /> : <Eye className="size-4" />}
          </button>
        </div>
      </Field>
      <Field id="url" label="Broker WebSocket URL" icon={Link2}>
        <Input id="url" value={form.url} onChange={set('url')} className="font-mono text-xs" required />
      </Field>
      <Field id="base" label="Topic base (shared by all boards)" icon={Tag}>
        <Input id="base" value={form.base} onChange={set('base')} className="font-mono text-xs" required />
      </Field>
      <p className="text-xs text-muted-foreground">
        Use the HiveMQ credential made for this web app, not the device one. Saved only in this browser.
      </p>
      <div className="flex flex-wrap justify-end gap-2">
        {onForget && (
          <Button type="button" variant="ghost" onClick={onForget}>
            Forget & disconnect
          </Button>
        )}
        <Button type="submit">{submitLabel}</Button>
      </div>
    </form>
  )
}

function Field({
  id,
  label,
  icon: Icon,
  children,
}: {
  id: string
  label: string
  icon: typeof User
  children: React.ReactNode
}) {
  return (
    <div className="grid gap-1.5">
      <Label htmlFor={id} className="flex items-center gap-1.5">
        <Icon className="size-3.5 text-muted-foreground" /> {label}
      </Label>
      {children}
    </div>
  )
}
