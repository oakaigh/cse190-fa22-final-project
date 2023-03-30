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

## Licensing
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
