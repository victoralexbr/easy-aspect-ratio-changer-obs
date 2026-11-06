[CmdletBinding()]
param(
    [ValidateSet('x64')]
    [string] $Target = 'x64',
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo'
)

$ErrorActionPreference = 'Stop'

if ( $DebugPreference -eq 'Continue' ) {
    $VerbosePreference = 'Continue'
    $InformationPreference = 'Continue'
}

if ( $env:CI -eq $null ) {
    throw "Package-Windows.ps1 requires CI environment"
}

if ( ! ( [System.Environment]::Is64BitOperatingSystem ) ) {
    throw "Packaging script requires a 64-bit system to build and run."
}

if ( $PSVersionTable.PSVersion -lt '7.2.0' ) {
    Write-Warning 'The packaging script requires PowerShell Core 7. Install or upgrade your PowerShell version: https://aka.ms/pscore6'
    exit 2
}

function Package {
    trap {
        Write-Error $_
        exit 2
    }

    $ScriptHome = $PSScriptRoot
    $ProjectRoot = Resolve-Path -Path "$PSScriptRoot/../.."
    $BuildSpecFile = "${ProjectRoot}/buildspec.json"

    $UtilityFunctions = Get-ChildItem -Path $PSScriptRoot/utils.pwsh/*.ps1 -Recurse

    foreach( $Utility in $UtilityFunctions ) {
        Write-Debug "Loading $($Utility.FullName)"
        . $Utility.FullName
    }

    $BuildSpec = Get-Content -Path ${BuildSpecFile} -Raw | ConvertFrom-Json
    $ProductName = $BuildSpec.name
    $ProductVersion = $BuildSpec.version

    $OutputName = "${ProductName}-${ProductVersion}-windows-${Target}"

    $RemoveArgs = @{
        ErrorAction = 'SilentlyContinue'
        Path = @(
            "${ProjectRoot}/release/${ProductName}-*-windows-*.zip"
        )
    }

    Remove-Item @RemoveArgs

    Log-Group "Archiving ${ProductName}..."

    # Layout do zip espelha a pasta de instalação do OBS: ao descompactar
    # em C:\Program Files\obs-studio\ o plugin já fica no lugar certo —
    # 64bit\*.dll -> obs-plugins\64bit\ e data\obs-plugins\<plugin>\ ...
    $InstallTree = "${ProjectRoot}/release/${Configuration}/${ProductName}"
    $Stage = "${ProjectRoot}/release/_staging"

    Remove-Item -Path $Stage -Recurse -Force -ErrorAction 'SilentlyContinue'
    New-Item -Path "${Stage}/64bit" -ItemType Directory -Force | Out-Null
    New-Item -Path "${Stage}/data/obs-plugins/${ProductName}" -ItemType Directory -Force | Out-Null

    Copy-Item -Path "${InstallTree}/bin/64bit/*" -Destination "${Stage}/64bit" -Recurse -Force
    Copy-Item -Path "${InstallTree}/data/*" -Destination "${Stage}/data/obs-plugins/${ProductName}/" -Recurse -Force

    $ReadMe = @(
        "Easy Aspect Ratio Changer for OBS Studio ${ProductVersion}",
        "",
        "",
        "Instalacao (Windows):",
        "",
        "1. Descompacte este zip dentro da pasta de instalacao do OBS Studio.",
        "   Normalmente: C:\Program Files\obs-studio",
        "2. Confirme a mesclagem de pastas quando o Windows perguntar.",
        "   * Conteudo de 64bit\ vai para obs-plugins\64bit\",
        "   * Pasta data\ vai para data\obs-plugins\<plugin>\",
        "3. Reinicie o OBS Studio e abra em Exibir > Docks > easy-aspect-ratio.",
        "",
        "Installation (English):",
        "",
        "1. Extract this zip into your OBS Studio installation folder",
        "   (usually C:\Program Files\obs-studio\).",
        "2. Allow folder merging when asked, then restart OBS Studio.",
        "3. Open View > Docks > easy-aspect-ratio.",
        "",
        "https://github.com/victoralexbr/easy-aspect-ratio-changer-obs"
    ) -join "`r`n"

    Set-Content -Path "${Stage}/README.txt" -Value $ReadMe -Encoding UTF8

    $CompressArgs = @{
        Path = "${Stage}/*"
        CompressionLevel = 'Optimal'
        DestinationPath = "${ProjectRoot}/release/${OutputName}.zip"
        Verbose = ($Env:CI -ne $null)
    }
    Compress-Archive -Force @CompressArgs

    Remove-Item -Path $Stage -Recurse -Force -ErrorAction 'SilentlyContinue'
    Log-Group
}

Package
