# project-4-oakaigh

## Device States
- **[init]**:
	- is device stationary for one minute?
		- `true`: enter state **[lost]**
		- `false`: `continue`
- **[lost]**: 
	- enable bluetooth broadcast
	- periodically send `:`-delimited status message over UART (see below)
	- magic command from UART ?
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
