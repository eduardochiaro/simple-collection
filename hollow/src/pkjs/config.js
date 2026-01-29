module.exports = [
  {
    "type": "heading",
    "defaultValue": "Simple Hollow Settings"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Display Options"
      },
      {
        "type": "color",
        "messageKey": "HOUR_COLOR",
        "defaultValue": "000000",
        "label": "Hours Color",
        "sunlight": true,
        "allowGray": true,
        "capabilities": ["COLOR"]
      },
      {
        "type": "color",
        "messageKey": "MINUTES_COLOR",
        "defaultValue": "000000",
        "label": "Minutes Color",
        "sunlight": true,
        "allowGray": true,
        "capabilities": ["COLOR"]
      },
      {
        "type": "color",
        "messageKey": "BACKGROUND_COLOR",
        "defaultValue": "FFFFFF",
        "label": "Background Color",
        "sunlight": true,
        "allowGray": true,
        "capabilities": ["COLOR"]
      },
      {
        "type": "toggle",
        "messageKey": "USE_SQUARE",
        "label": "Use Square design",
        "description": "Switch between square and round design.",
        "defaultValue": false,
        "capabilities": ["RECT"]
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];