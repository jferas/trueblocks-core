## grabABI

`grabABI` retrieves an ABI file either from a local cache, from the Ethereum Name Service (ENS), or from [Etherscan](http://etherscan.io). Once retrieved, `grabABI` generates the classDefinition files needed by [makeClass](../makeClass/README.md) to generate a parselib for each [Ethereum address monitor](../../monitors/README.md).

#### Usage

`Usage:`    grabABI [-c|-g|-d|-e|-j|-n|-o|-l|-v|-h] addr  
`Purpose:`  Fetches the ABI for a smart contract. Optionally generates C++ source code representing that ABI.
             
`Where:`  

| Short Cut | Option | Description |
| -------: | :------- | :------- |
|  | addr | the address(es) of the smart contract(s) to grab |
| -c | --canonical | convert all types to their canonical represenation and remove all spaces from display |
| -g | --generate | generate C++ code into the current folder for all functions and events found in the ABI |
| -d | --data | export the display as data |
| -e | --encode | generate the encodings for the functions / events in the ABI |
| -j | --json | print the ABI to the screen as json |
| -n | --noconst | generate encodings for non-constant functions and events only (always true when generating) |
| -o | --open | open the ABI file for editing, download if not already present |
| -l | --sol fn | create the ABI file from a .sol file in the local directory |
| -v | --verbose | set verbose level. Either -v, --verbose or -v:n where 'n' is level |
| -h | --help | display this help screen |

`Notes:`

- Use the `--silent` option, which displays fewer messages, for scripting.

#### Other Options

All **QBlocks** command-line tools support the following commands (although in some case, they have no meaning):

    Command     |     Description
    -----------------------------------------------------------------------------
    --version   |   display the current version of the tool
    --nocolor   |   turn off colored display
    --wei       |   specify value in wei (the default)
    --ether     |   specify value in ether
    --dollars   |   specify value in US dollars
    --raw       |   report JSON data from the node with minimal processing
    --veryRaw   |   report JSON data from node with zero processing
    --fmt       |   export format (where appropriate). One of [none|txt|csv|json|api]
    --api_mode  |   simulate api_mode for testing
    --file:fn   |   specify multiple sets of command line options in a file.

<small>*For the `--file:fn` option, place a series of valid command lines in a file and use the above options. In some cases, this option may significantly improve performance. A semi-colon at the start of a line makes that line a comment.*</small>

**Powered by Qblocks<sup>&trade;</sup>**


