# LogManager

LogManager is a Qt-based application for viewing and analyzing log files. It supports custom log formats defined in JSON and can merge log entries from multiple modules.

## Features

- Parse logs according to user-defined formats.
- Load individual log files or scan entire folders.
- Filter entries by module and time range.
- Display logs using a table with on-demand fetching.
- Apply per-column filters directly in the table view.
- Manage log formats (add or remove) via JSON descriptions.

## Branches

There are two branches in this repository. The default branch builds a Qt-only
version with no additional dependencies. The alternative branch adds archive
support using the [QuaZip](https://github.com/stachenov/quazip) library and can
open log files stored in ZIP archives.

## Building

This project uses CMake and requires Qt&nbsp;6 and a C++20 capable compiler.

## Format Definitions

Log formats are described with JSON files. Each file specifies modules, file extensions, encoding, comment styles, field separators and regular expressions for extracting data. Formats are loaded at startup and can be added or removed using the GUI.

## Running

Execute the built binary to start the GUI. Use the **File** menu to open log files or folders and select the active formats. After scanning, choose a time range and modules to display.
