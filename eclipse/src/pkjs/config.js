module.exports = [
  {
    "type": "heading",
    "defaultValue": "Simple Eclipse Settings"
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
        "messageKey": "HOURS_COLOR",
        "defaultValue": "FFFFFF",
        "label": "Hours Color",
        "sunlight": true,
        "allowGray": true,
        "capabilities": ["COLOR"]
      },
      {
        "type": "toggle",
        "messageKey": "INVERT_COLORS",
        "label": "Invert Colors",
        "description": "Switch between light and dark theme.",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "USE_SQUARE",
        "label": "Use Square design",
        "description": "Switch between square and round design.",
        "defaultValue": false,
        "capabilities": ["RECT"],
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];