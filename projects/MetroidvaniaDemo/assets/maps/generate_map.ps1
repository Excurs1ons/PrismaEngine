# Generate test_dungeon.json — 3 rooms × 40 tiles wide, 30 tiles tall
param(
    [int]$Rooms = 3,
    [int]$RoomW = 40,
    [int]$H = 30,
    [int]$TileSize = 16,
    [string]$Output = "test_dungeon.json"
)

$W = $Rooms * $RoomW  # 120

# Build tile data: 0 = empty, 1 = solid
$tiles = [int[]]::new($W * $H)

function Set-Tile($x, $y, $val=1) {
    if ($x -ge 0 -and $x -lt $W -and $y -ge 0 -and $y -lt $H) {
        $tiles[$y * $W + $x] = $val
    }
}

function Fill-Rect($x1, $y1, $x2, $y2, $val=1) {
    for ($y = $y1; $y -le $y2; $y++) {
        for ($x = $x1; $x -le $x2; $x++) {
            Set-Tile $x $y $val
        }
    }
}

# === Boundary walls ===
Fill-Rect 0 0 ($W-1) 0          # Top wall (y=0)
Fill-Rect 0 ($H-1) ($W-1) ($H-1) # Bottom floor (y=29)
Fill-Rect 0 1 0 ($H-2)          # Left wall (x=0)
Fill-Rect ($W-1) 1 ($W-1) ($H-2) # Right wall (x=119)

# === Room 1 (x=0..39): Starting area ===
# Floor ramp / step platforms
Fill-Rect 5 27 10 27          # Low step (near spawn)
Fill-Rect 14 25 19 25         # Mid step
Fill-Rect 24 23 30 23         # Higher step
Fill-Rect 32 20 38 20         # High platform leading to room 2

# Small floating platforms
Fill-Rect 8 19 12 19          # Floating platform (practice)
Fill-Rect 20 15 24 15         # Higher floating platform
Fill-Rect 28 12 32 12         # Tall floating platform

# === Room 2 (x=40..79): Dash gap area ===
# Floor continues, but with gap x=56..65 (10 tiles = 160px, not jumpable)
Fill-Rect 40 29 55 29         # Floor before gap
# GAP: x=56..65 — no floor tiles
Fill-Rect 66 29 79 29         # Floor after gap

# Platform in the gap (stepping stone with dash pickup)
Fill-Rect 60 25 61 25         # Mid-gap platform (dash pickup here)

# Platforms leading to gap
Fill-Rect 44 25 49 25         # Approach platform (slightly raised)
Fill-Rect 68 25 72 25         # Landing platform after gap

# Some ceiling decorations / platforms in room 2
Fill-Rect 50 18 55 18         # High platform above approach
Fill-Rect 70 16 75 16         # High platform after gap

# === Room 3 (x=80..119): End area ===
Fill-Rect 80 29 119 29        # Floor (already set, but explicit)
Fill-Rect 82 25 87 25         # Platform
Fill-Rect 90 22 95 22         # Higher platform
Fill-Rect 98 18 103 18        # Higher still
Fill-Rect 106 14 111 14       # High platform near end
Fill-Rect 115 25 118 25       # End platform before right wall

# === Generate JSON ===
$tileList = @($tiles -join ",")

$json = @"
{
  "tileSize": $TileSize,
  "columns": 8,
  "texturePath": "",
  "tiles": [
    { "tileId": 1, "texIndex": 0, "collisionFlags": 1 }
  ],
  "layers": [
    {
      "name": "ground",
      "width": $W,
      "height": $H,
      "tiles": [$tileList],
      "visible": true,
      "sortingOrder": 0
    }
  ]
}
"@

$json | Out-File -Encoding utf8 $Output
Write-Host "Generated $Output ($($tiles.Count) tiles, $(($tiles | Where-Object {$_ -gt 0}).Count) solid)"
