# Deploy to bensclock

From WSL, run `~/src/denclock/deploy-to-bensclock.sh`. It works from any directory.
Install the .NET 10 SDK, OpenSSH client and rsync locally. The remote host needs
rsync and the .NET 10 ASP.NET Core runtime (including the base .NET runtime).
SSH access to ben@bensclock and sudo permission are required. By default, SSH
and sudo prompt interactively.

For password-file use, install `sshpass` locally (`sudo apt-get install sshpass`
on WSL/Debian). Create `.bensclock-password` beside the scripts with the shared
SSH/sudo password on one line. The file is excluded from Git. Create it privately:

```bash
(umask 077; read -rsp 'bensclock password: ' clock_password; printf '\n'; printf '%s\n' "$clock_password" > .bensclock-password)
chmod 600 .bensclock-password
```

Both scripts automatically use this file when present. It must be nonempty,
owned by your user, and mode 600. The password is stored as plaintext locally;
it is passed to sshpass by file and sudo through SSH standard input, never as a
command argument. SSH host-key verification remains enabled; establish trust
with an ordinary SSH connection first. Removing the file restores prompts.

Both projects are published in Release for net10.0/linux-x64, framework-dependent,
into a unique temporary directory removed on exit. Both publishes must succeed
before remote setup begins. Existing publish/ output is never used.
The script creates /opt/denclock/server and /opt/denclock/lora-service and
recursively assigns only these directories to ben and ben's primary group.

Full output is mirrored with rsync --delete. Published configuration replaces
deployed copies and destination-only files, including logs, are removed.
Keep persistent data outside these directories. On Linux the server requires
Presentation__ConfigurationDirectory pointing to a separate writable directory,
for example /var/lib/denclock/server. Override LoRa Logging__FilePath with an
external path such as /var/log/denclock/lora-service-.log. Provision those
directories with permissions for the service account. MQTT, runtime directories,
runtimes and systemd units are not provisioned by this script.

Stop any running applications before deploying. Transfers are not atomic across
both services. Errors stop further steps and can leave partial remote output;
correct the problem and rerun. There is no rollback or automatic stop.

No services restart by default. Once units exist, pass their actual names
explicitly, repeating --restart-unit for each. For example:

```sh
./deploy-to-bensclock.sh --restart-unit denclock-server.service --restart-unit denclock-lora.service
```

These names match the units in the Surface Go installation guide. To deploy and
restart both with one command, use the wrapper (it works from any directory):

```sh
./deploy-and-restart-bensclock.sh
```

The wrapper calls the existing deployment script with both restart units, then
connects over SSH as ben and sends Ctrl+Shift+R to the visible Chromium kiosk on
X11 display :0. Install its refresh dependency once on bensclock with
`sudo apt-get install xdotool`. The graphical session must allow ben to access
that display. Deployment or restart failure prevents the refresh; a refresh
failure also returns an error, even though deployment has already succeeded.

Restarts occur only after both
transfers succeed; restart or active-state verification failures return an error.
Units are not installed or enabled. Existing Windows helpers remain available.
