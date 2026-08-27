"""Secret-masking patterns shared by every script in this project.

There is exactly one definition of what a secret looks like, and it is used
BOTH to mask and to verify the masking afterwards. If those two ever diverge
the verification becomes theatre, so they share this list.

Two scripts import this:

  export-transcript.sh   masks a rendered session log
  read-banner.sh         masks the board's boot output as it is displayed

Duplicating the patterns into the second script was the obvious move and the
wrong one -- the whole reason this file exists is that the SSID leaked twice,
the second time because one copy of the rules was less complete than the other.

A pattern is (label, regex). The regex must capture the key in group 1 and the
value in group 2, because masking replaces the match with group(1) + '***'.
The (?!\\*) guard stops an already-masked value from matching again, which is
what lets the residual check mean something.
"""

import hashlib
import re

PATTERNS = [
    ('channel URL',
     r'(https://meshtastic\.org/e/#)[A-Za-z0-9_\-]{20,}'),
    ('wifi psk (echoed)',
     r'(wifi[_ ]?psk\s*(?:to|=|:)\s+)(?!\*)(\S{4,})'),
    ('wifi psk (argument)',
     r'(--set\s+network\.wifi_psk\s+)(?!\*)([\'"]?[^\'"\s]{4,}[\'"]?)'),
    ('wifiPsk (yaml)',
     r'(wifiPsk\s*:\s+)(?!\*)(\S{4,})'),
    ('psk (yaml)',
     r'(\bpsk\s*:\s+)(?!\*)([A-Za-z0-9+/=]{16,})'),
    ('psk (hex arg)',
     r'(--ch-set\s+psk\s+)(0x[0-9a-fA-F]{16,})'),
    ('mqtt password',
     r'(mqtt\.password\s+)(?!\*)(\S{4,})'),
    # security.privateKey from `meshtastic --export-config`. This is the node's
    # PKI identity: it decrypts direct messages addressed to us and can be used
    # to impersonate the node. It leaked into a session on 2026-08-19 because
    # nothing here matched it.
    ('node private key',
     r'(privateKey\s*:\s*(?:base64:)?)(?!\*)([A-Za-z0-9+/=]{16,})'),
    # Not a secret, but it names the operator's home network.
    ('wifi ssid (yaml)',
     r'(wifiSsid\s*:\s+)(?!\*)(\S+)'),
    # Config dumps printed by third-party firmware. The ESP-1ch-Gateway sketch
    # prints its stored WiFi credentials on every boot as ".SSID=" / ".PASS=".
    ('firmware config dump (pass)',
     r'(^\s*\.?PASS\s*=\s*)(?!\*)(\S+)'),
    ('firmware config dump (ssid)',
     r'(^\s*\.?SSID\s*=\s*)(?!\*)(\S+)'),
    # The same sketch prints the SSID AGAIN at runtime, on lines the anchored
    # patterns above cannot reach because text precedes the key:
    #   "0:1:2. WiFi connect SSID=<ssid>"      src/_WiFi.ino:254 (installed build)
    #   "WlanStatus:: CONNECTED ssid=<ssid>"   src/_WiFi.ino:37
    #   "Set Variables:: ssid=<ssid>"          src/_wwwServer.ino:479
    # One unanchored key beats one pattern per phrasing. Over-masking a
    # transcript costs a little readability; under-masking costs a leak.
    ('wifi ssid (inline key)',
     r'(?i)(\bssid\s*=\s*)(?!\*)(\S+)'),
    # src/_WiFi.ino:257 appends ", pass=<password>" to that same runtime line
    # when the stored debug level is >= 2. This board stores debug=1, so it has
    # not appeared yet -- one config change is all that stands between the
    # password and every future capture.
    ('firmware wifi pass (inline key)',
     r'(?i)(\bpass\s*=\s*)(?!\*)(\S+)'),
    # The SSID also appears as the value of two differently-named keys:
    #   "setup:: WlanConnect=<ssid>"                 src/ESP-sc-gway.ino:420
    #   "sendUdp: WlanConnect connected to=<ssid>"   src/_udpSemtech.ino:603
    ('wifi ssid (WlanConnect=)',
     r'(WlanConnect\s*=\s*)(?!\*)(\S+)'),
    ('wifi ssid (connected to=)',
     r'(?i)(connected to\s*=\s*)(?!\*)(\S+)'),
    ('wifi ssid (argument)',
     r'(--set\s+network\.wifi_ssid\s+)(?!\*)([\'"]?[^\'"\s]+[\'"]?)'),

    # --- The 2019 firmware actually on this board (ESP-1ch-Gateway-v5.0) ---
    # Everything above was derived from things4u/ESP-1ch-Gateway, which is a
    # DIFFERENT and later repository than the one SparkFun links. The build on
    # this board is v5.0, and it prints credentials in shapes with no key in
    # front of the value, which no key-based pattern can reach.
    #
    # _WiFi.ino:155  "WlanReadWpa: <ssid>, <password>"
    # _WiFi.ino:160  "WlanReadWpa: <<ssid>>, <<password>>"
    # The password is separated from the SSID by nothing but ", ". The only
    # reliable handle is the line prefix, so mask the whole rest of the line.
    # The guard must sit BEFORE the whitespace, not after it. With
    # r'(WlanReadWpa:\s*)(?!\*)(.+)' the greedy \s* backtracks to empty, the
    # lookahead then sees a space instead of the '*', and an already-masked
    # line matches a second time -- which makes residual() report a leak that
    # is not there, and read-banner.sh treats any residual as fatal.
    ('firmware WlanReadWpa (rest of line)',
     r'(WlanReadWpa:)(?!\s*\*\*\*)\s*(.+)'),
    # _WiFi.ino:39   "A WlanStatus:: CONNECTED to <ssid>"
    # ESP-sc-gway.ino:1112  " WiFi Connected to <ssid> on IP="
    # "to", not "to=" -- the (connected to=) pattern above requires an equals
    # sign and sails straight past both of these.
    ('wifi ssid (connected to)',
     r'(?i)(\bconnected to\s+)(?!\*)(\S+)'),
    # _WiFi.ino:178  "M WlanWriteWpa:: ssid=<ssid>, pass=<password>" is caught
    # by the inline-key patterns above, but note its guard is debug>=0 in v5.0
    # rather than debug>=2 -- it prints whenever the config is written.
]


def mask(body, extra=()):
    """Mask every pattern, then every literal in `extra`.

    Returns (masked_body, {label: count}). re.M is needed because the
    firmware config-dump patterns are anchored per line.
    """
    counts = {}
    for label, pat in PATTERNS:
        body, n = re.subn(pat, lambda m: m.group(1) + '***', body, flags=re.M)
        if n:
            counts[label] = n
    for literal in extra:
        n = body.count(literal)
        if n:
            body = body.replace(literal, '***')
            counts[_label(literal)] = n
    return body, counts


def _label(literal):
    """A stable name for a literal that does not disclose any of it.

    This used to be f'literal {literal[:6]}…'. That prints the first six
    characters of a secret into the very report that claims to have masked it,
    which on 2026-08-21 disclosed most of a ten-digit password. A truncated
    hash names the literal just as well and reveals nothing.
    """
    digest = hashlib.sha256(literal.encode()).hexdigest()[:8]
    return f'literal sha:{digest} ({len(literal)} chars)'


def residual(body):
    """Anything still matching after mask() is a real leak, not a false
    positive, because every pattern requires a value to follow the key."""
    out = {}
    for label, pat in PATTERNS:
        hits = re.findall(pat, body, flags=re.M)
        if hits:
            out[label] = len(hits)
    return out


def load_literals(path):
    """Read a secrets file: one literal per line, '#' comments, blanks ignored.

    Literals catch what no pattern can -- a bare SSID with no "ssid=" in front
    of it, in prose or in a string literal. Missing file is not an error.
    """
    try:
        with open(path, encoding='utf-8') as fh:
            return [ln.strip() for ln in fh
                    if ln.strip() and not ln.lstrip().startswith('#')]
    except FileNotFoundError:
        return []
