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
        "messageKey": "HOURS_COLOR",
        "defaultValue": "000000",
        "label": "Hours Color",
        "sunlight": true,
        "allowGray": true
      },
      {
        "type": "color",
        "messageKey": "HOURS_OVERLAY_COLOR",
        "defaultValue": "8EE69E",
        "label": "Hours Overlay Color",
        "sunlight": true,
        "allowGray": true
      },
      {
        "type": "color",
        "messageKey": "MINUTES_COLOR",
        "defaultValue": "000000",
        "label": "Minutes Color",
        "sunlight": true,
        "allowGray": true
      },
      {
        "type": "color",
        "messageKey": "MINUTES_OVERLAY_COLOR",
        "defaultValue": "8EE69E",
        "label": "Minutes Overlay Color",
        "sunlight": true,
        "allowGray": true
      },
      {
        "type": "color",
        "messageKey": "BACKGROUND_COLOR",
        "defaultValue": "FFFFFF",
        "label": "Background Color",
        "sunlight": true,
        "allowGray": true
      },
      {
        "type": "toggle",
        "messageKey": "USE_RECT",
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
