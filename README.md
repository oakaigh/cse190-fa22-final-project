# project-4-oakaigh

## Device States
- **[init]**:
	- Device stationary for one minute?
		- `true`: enter state **[lost]**
		- `false`: `continue`
- **[lost]**: 
	- Enable bluetooth broadcast
	- Periodically send `:`-delimited status message over UART (see below)
	- Magic command from UART received?
		- `found`: leave state **[lost]**
	- `continue`

## Interface

### Bluetooth (BLE) UART
- Output: format `<device_name>:<n_minutes_lost>`.
- Input: commands: `found`.

## Housekeeping
```sh
{
	curl -L https://github.com/github/gitignore/raw/main/{C,C++,Global/{Linux,Windows,macOS,Vim,SublimeText,VisualStudioCode}}.gitignore
	cat <<- "EOF"
	# User Defined
	build
	EOF
} > .gitignore
```
