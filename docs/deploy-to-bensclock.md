# Deploy to bensclock

From WSL, run `~/src/denclock/deploy-to-bensclock.sh`. It works from any directory.
Install the .NET 10 SDK, OpenSSH client and rsync locally. The remote host needs
rsync and the .NET 10 ASP.NET Core runtime (including the base .NET runtime).
SSH access to ben@bensclock and sudo permission are required. ssh -t allows sudo
to prompt for a password without passing passwords through the script.

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

These are example names, not installed units. Restarts occur only after both
transfers succeed; restart or active-state verification failures return an error.
Units are not installed or enabled. Existing Windows helpers remain available.
