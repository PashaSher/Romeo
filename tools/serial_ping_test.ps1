# Обёртка: на многих ПК .NET SerialPort с USB CDC (Leonardo) не получает ответы.
# Реальная проверка — через pyserial (тот же код, что и в Python).
param(
  [string]$Port = "COM14"
)
$py = Join-Path $PSScriptRoot "serial_ping_test.py"
if (-not (Test-Path $py)) {
  Write-Error "Не найден $py"
  exit 1
}
python $py $Port
