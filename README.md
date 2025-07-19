# LogManager

LogManager is a Qt-based application for viewing and analyzing log files. It
supports custom log formats defined in JSON and can merge log entries from
multiple modules.

## Features

- Parse logs according to user-defined formats.
- Load individual log files or scan entire folders.
- Filter entries by module and time range.
- Display logs using a table with on-demand fetching.
- Apply per-column filters directly in the table view.
- Manage log formats (add or remove) via JSON descriptions.
- Perform local or global search with optional regular expressions.
- Export results to CSV files or the original log format.

## Building

This project uses CMake and requires Qt&nbsp;6 and a C++20 capable compiler.

## Format Definitions

Log formats are described with JSON files. Each file specifies modules, file
extensions, encoding, comment styles and either field separators or a regular
expression for the entire line. Formats are loaded at startup and can be added
or removed using the GUI.
Each JSON file may contain the following keys:

- `modules` - names of modules present in the log. This list is optional and used
  to detect if file matches this format.
- `logFileRegex` - regular expression that matches log file names. It can contain
  named capturing group `module` which is used for filtering.
- `extension` - the log file extension associated with this format. Only files
  using this extension will be interpreted with the format, and the value must
  include the leading dot such as `.log`.
- `encoding` - text encoding such as `UTF-8`.
- `comments` - array of objects with `start` and optional `finish` strings
  describing line comments.
- `separator` - delimiter separating fields in a log line.
- `lineRegex` - regular expression describing the entire log line when no
  separator is provided. Capturing groups correspond to fields in order.
- `timeFieldIndex` - zero-based index of the timestamp field.
- `timeRegex` - pattern used to parse the timestamp. Std::chrono::parse format is used.
- `fields` - array of field definitions. Each entry contains a `name`, a
  regular expression `regex` and the Qt type for the value.

An example format file looks like this:

```json
{
  "modules": ["Core", "UI"],
  "logFileRegex": "log-(?<module>[[:alnum:]]*)-(?<time>\\d*)",
  "extension": ".log",
  "encoding": "UTF-8",
  "comments": [{"start": "#"}],
  "separator": ",",
  "timeFieldIndex": 0,
  "timeRegex": "%T %F",
  "fields": [
    {"name": "time", "regex": "^(\\d{4}-\\d{2}-\\d{2}) (\\d{2}:\\d{2}:\\d{2}.\\d{3})", "type": "QString"},
    {"name": "level", "regex": "([A-Z]+)", "type": "QString"},
    {"name": "message", "regex": "(.*)", "type": "QString"}
  ]
}
```

## Running

Execute the binary to start the GUI. Access the **Format** menu to manage the list of available formats.
Use the **File** menu to open log files or folders. After scanning, choose a time range and modules to display.
