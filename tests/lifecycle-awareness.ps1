param(
    [Parameter(Mandatory = $true)]
    [string] $Executable
)

$ErrorActionPreference = 'Stop'
if (-not ('TrafficMonitorVersionResource' -as [type])) {
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class TrafficMonitorVersionResource {
    [DllImport("version.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern uint GetFileVersionInfoSize(string path, out uint handle);
    [DllImport("version.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern bool GetFileVersionInfo(string path, uint handle, uint size, byte[] data);
    [DllImport("version.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern bool VerQueryValue(byte[] data, string block, out IntPtr value, out uint length);
}
'@
}

$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
[uint32] $ignoredHandle = 0
$resourceSize = [TrafficMonitorVersionResource]::GetFileVersionInfoSize($resolvedExecutable, [ref] $ignoredHandle)
# Squirrel.Windows 2.0.1 rejects larger resources before its fixed-language lookup.
if ($resourceSize -eq 0 -or $resourceSize -gt 4096) {
    throw "Squirrel.Windows 2.0.1 awareness rejected: version resource size is $resourceSize (allowed 1..4096)."
}
$resource = [byte[]]::new($resourceSize)
if (-not [TrafficMonitorVersionResource]::GetFileVersionInfo($resolvedExecutable, 0, $resourceSize, $resource)) {
    throw 'Cannot read executable version resource.'
}
$valuePointer = [IntPtr]::Zero
[uint32] $valueLength = 0
if (-not [TrafficMonitorVersionResource]::VerQueryValue($resource,
    '\StringFileInfo\040904B0\SquirrelAwareVersion', [ref] $valuePointer, [ref] $valueLength)) {
    throw 'Squirrel.Windows 2.0.1 awareness rejected: fixed English resource key is absent.'
}
# The pinned detector returns 1 on successful lookup; it does not parse the value.
Write-Output "PASS: Squirrel.Windows 2.0.1 native awareness (resource bytes: $resourceSize)."
