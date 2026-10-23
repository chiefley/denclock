# Sends an mDNS A query directly, bypassing Windows name resolution.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Hostname,
    [Parameter(Mandatory=$true)][System.Net.IPAddress]$InterfaceAddress,
    [Parameter(Mandatory=$true)][System.Net.IPAddress]$ExpectedAddress,
    [ValidateRange(1,30)][int]$TimeoutSeconds = 5,
    [switch]$LegacyUnicast
)
$ErrorActionPreference = 'Stop'
if ($InterfaceAddress.AddressFamily -ne [System.Net.Sockets.AddressFamily]::InterNetwork -or
    $ExpectedAddress.AddressFamily -ne [System.Net.Sockets.AddressFamily]::InterNetwork) {
    throw 'Use IPv4 LAN addresses.'
}
$name = $Hostname.TrimEnd('.').ToLowerInvariant()
if ($name -notmatch '^[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?\.local$') {
    throw 'Use one hostname label followed by .local.'
}

# DNS names may use compression pointers. Bound all reads and pointer traversal.
function Read-DnsName([byte[]]$Data, [ref]$Offset) {
    $pos = [int]$Offset.Value
    $next = -1
    $labels = [System.Collections.Generic.List[string]]::new()
    for ($steps = 0; $steps -lt 128; $steps++) {
        if ($pos -ge $Data.Length) { throw 'Truncated DNS name' }
        $length = [int]$Data[$pos]
        $pos++
        if ($length -eq 0) {
            if ($next -lt 0) { $next = $pos }
            $Offset.Value = $next
            return ($labels -join '.').ToLowerInvariant()
        }
        if (($length -band 192) -eq 192) {
            if ($pos -ge $Data.Length) { throw 'Truncated DNS pointer' }
            if ($next -lt 0) { $next = $pos + 1 }
            $pos = (($length -band 63) * 256) + $Data[$pos]
        } else {
            if ($length -gt 63 -or $pos + $length -gt $Data.Length) { throw 'Invalid DNS label' }
            $labels.Add([System.Text.Encoding]::ASCII.GetString($Data, $pos, $length))
            $pos += $length
        }
    }
    throw 'DNS pointer loop or excessive name length'
}
function Read-U16([byte[]]$Data, [ref]$Offset) {
    $pos = [int]$Offset.Value
    if ($pos + 2 -gt $Data.Length) { throw 'Truncated DNS integer' }
    $Offset.Value = $pos + 2
    return ([int]$Data[$pos] * 256 + $Data[$pos + 1])
}

$id = 0
if ($LegacyUnicast) { $id = Get-Random -Minimum 1 -Maximum 65536 }
$query = [System.Collections.Generic.List[byte]]::new()
$query.AddRange([byte[]]@(($id -shr 8), ($id -band 255), 0, 0, 0, 1, 0, 0, 0, 0, 0, 0))
foreach ($label in $name.Split('.')) {
    $query.Add([byte]$label.Length)
    $query.AddRange([System.Text.Encoding]::ASCII.GetBytes($label))
}
$query.AddRange([byte[]]@(0, 0, 1, 0, 1)) # root, A, IN
$socket = [System.Net.Sockets.UdpClient]::new([System.Net.Sockets.AddressFamily]::InterNetwork)
$found = [System.Collections.Generic.HashSet[string]]::new()
try {
    $socket.ExclusiveAddressUse = $false
    $socket.Client.SetSocketOption([System.Net.Sockets.SocketOptionLevel]::Socket,
        [System.Net.Sockets.SocketOptionName]::ReuseAddress, $true)
    if ($LegacyUnicast) {
        $socket.Client.Bind([System.Net.IPEndPoint]::new($InterfaceAddress, 0))
    } else {
        $socket.Client.Bind([System.Net.IPEndPoint]::new([System.Net.IPAddress]::Any, 5353))
        $socket.JoinMulticastGroup([System.Net.IPAddress]::Parse('224.0.0.251'), $InterfaceAddress)
    }
    $socket.Client.SetSocketOption([System.Net.Sockets.SocketOptionLevel]::IP,
        [System.Net.Sockets.SocketOptionName]::MulticastInterface, $InterfaceAddress.GetAddressBytes())
    $socket.Client.SetSocketOption([System.Net.Sockets.SocketOptionLevel]::IP,
        [System.Net.Sockets.SocketOptionName]::MulticastTimeToLive, 255)
    $destination = [System.Net.IPEndPoint]::new([System.Net.IPAddress]::Parse('224.0.0.251'), 5353)
    $bytes = $query.ToArray()
    [void]$socket.Send($bytes, $bytes.Length, $destination)
    Write-Host "Query: $name A IN via $InterfaceAddress -> 224.0.0.251:5353 (ID $id)"
    $watch = [System.Diagnostics.Stopwatch]::StartNew()
    while ($watch.Elapsed.TotalSeconds -lt $TimeoutSeconds) {
        $socket.Client.ReceiveTimeout = [Math]::Max(1, [int](1000 * ($TimeoutSeconds - $watch.Elapsed.TotalSeconds)))
        $source = [System.Net.IPEndPoint]::new([System.Net.IPAddress]::Any, 0)
        try { $data = $socket.Receive([ref]$source) }
        catch [System.Net.Sockets.SocketException] {
            if ($_.Exception.SocketErrorCode -eq [System.Net.Sockets.SocketError]::TimedOut) { break }
            throw
        }
        try {
            $offset = 0
            if ((Read-U16 $data ([ref]$offset)) -ne $id) { continue }
            $flags = Read-U16 $data ([ref]$offset)
            if (($flags -band 32768) -eq 0 -or ($flags -band 15) -ne 0 -or $source.Port -ne 5353) { continue }
            $questions = Read-U16 $data ([ref]$offset)
            $records = Read-U16 $data ([ref]$offset)
            $records += Read-U16 $data ([ref]$offset)
            $records += Read-U16 $data ([ref]$offset)
            for ($i = 0; $i -lt $questions; $i++) {
                $null = Read-DnsName $data ([ref]$offset)
                $null = Read-U16 $data ([ref]$offset)
                $null = Read-U16 $data ([ref]$offset)
            }
            for ($i = 0; $i -lt $records; $i++) {
                $owner = Read-DnsName $data ([ref]$offset)
                $type = Read-U16 $data ([ref]$offset)
                $class = Read-U16 $data ([ref]$offset)
                $ttlHigh = Read-U16 $data ([ref]$offset)
                $ttlLow = Read-U16 $data ([ref]$offset)
                $length = Read-U16 $data ([ref]$offset)
                if ($offset + $length -gt $data.Length) { throw 'Truncated DNS record' }
                if ($owner -eq $name -and $type -eq 1 -and ($class -band 32767) -eq 1 -and
                    $length -eq 4 -and ($ttlHigh -ne 0 -or $ttlLow -ne 0)) {
                    $address = ([System.Net.IPAddress]::new([byte[]]$data[$offset..($offset + 3)])).ToString()
                    [void]$found.Add($address)
                    Write-Host "Answer: $owner A $address from $source"
                }
                $offset += $length
            }
        } catch { Write-Warning "Ignoring malformed response from ${source}: $_" }
    }
} finally { $socket.Dispose() }
if ($found.Count -eq 0) { throw "No mDNS A answer for $name. Check LAN, interface, responder, policy and firewall." }
if ($found.Count -ne 1 -or -not $found.Contains($ExpectedAddress.ToString())) {
    throw "Unexpected or ambiguous addresses: $($found -join ', '). Do not configure the gateway until ownership is resolved."
}
Write-Host "PASS: $name answered with expected LAN address $ExpectedAddress. Gateway multicast-path acceptance is still required."
