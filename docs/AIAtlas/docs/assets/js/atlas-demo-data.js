/* Сгенерировано tools/gen_demo_data.py из assets/ai и уровня Access Key. Не редактировать вручную. */
window.ATLAS_DEMO_DATA = {
 "snapshot": "8c6c0dc37ab3",
 "semanticIds": {
  "move_to": 2,
  "purchase": 3
 },
 "domains": {
  "access_key": {
   "title": "Access Key (реальные ассеты, route costs)",
   "goap": {
    "id": "access_key",
    "facts": [
     {
      "name": "hasAccessKey",
      "type": "bool"
     },
     {
      "name": "atDestination",
      "type": "bool"
     },
     {
      "name": "coinACollected",
      "type": "bool"
     },
     {
      "name": "coinBCollected",
      "type": "bool"
     },
     {
      "name": "coinCCollected",
      "type": "bool"
     },
     {
      "name": "atAccessKeyShop",
      "type": "bool"
     },
     {
      "name": "atStart",
      "type": "bool"
     },
     {
      "name": "atCoinA",
      "type": "bool"
     },
     {
      "name": "atCoinB",
      "type": "bool"
     },
     {
      "name": "atCoinC",
      "type": "bool"
     },
     {
      "name": "atGoal",
      "type": "bool"
     },
     {
      "name": "coinAAvailable",
      "type": "bool"
     },
     {
      "name": "coinBAvailable",
      "type": "bool"
     },
     {
      "name": "coinCAvailable",
      "type": "bool"
     },
     {
      "name": "coins",
      "type": "int"
     }
    ],
    "goals": [
     {
      "name": "reachDestination",
      "score": 1.0,
      "facts": [
       {
        "fact": "atDestination",
        "value": true
       }
      ]
     }
    ],
    "actions": [
     {
      "action": "move_to",
      "context": "start_to_coin_a",
      "cost": 1.0,
      "preconditions": [
       {
        "fact": "atStart",
        "value": true
       },
       {
        "fact": "coinACollected",
        "value": false
       },
       {
        "fact": "coinAAvailable",
        "value": true
       }
      ],
      "effects": [
       {
        "fact": "atStart",
        "value": false
       },
       {
        "fact": "atCoinA",
        "value": true
       },
       {
        "fact": "coinACollected",
        "value": true
       }
      ],
      "numericEffects": [
       {
        "fact": "coins",
        "op": "add",
        "value": 1
       }
      ]
     },
     {
      "action": "move_to",
      "context": "start_to_coin_b",
      "cost": 1.0,
      "preconditions": [
       {
        "fact": "atStart",
        "value": true
       },
       {
        "fact": "coinBCollected",
        "value": false
       },
       {
        "fact": "coinBAvailable",
        "value": true
       }
      ],
      "effects": [
       {
        "fact": "atStart",
        "value": false
       },
       {
        "fact": "atCoinB",
        "value": true
       },
       {
        "fact": "coinBCollected",
        "value": true
       }
      ],
      "numericEffects": [
       {
        "fact": "coins",
        "op": "add",
        "value": 1
       }
      ]
     },
     {
      "action": "move_to",
      "context": "start_to_coin_c",
      "cost": 1.0,
      "preconditions": [
       {
        "fact": "atStart",
        "value": true
       },
       {
        "fact": "coinCCollected",
        "value": false
       },
       {
        "fact": "coinCAvailable",
        "value": true
       }
      ],
      "effects": [
       {
        "fact": "atStart",
        "value": false
       },
       {
        "fact": "atCoinC",
        "value": true
       },
       {
        "fact": "coinCCollected",
        "value": true
       }
      ],
      "numericEffects": [
       {
        "fact": "coins",
        "op": "add",
        "value": 1
       }
      ]
     },
     {
      "action": "move_to",
      "context": "coin_a_to_coin_b",
      "cost": 1.0,
      "preconditions": [
       {
        "fact": "atCoinA",
        "value": true
       },
       {
        "fact": "coinBCollected",
        "value": false
       },
       {
        "fact": "coinBAvailable",
        "value": true
       }
      ],
      "effects": [
       {
        "fact": "atCoinA",
        "value": false
       },
       {
        "fact": "atCoinB",
        "value": true
       },
       {
        "fact": "coinBCollected",
        "value": true
       }
      ],
      "numericEffects": [
       {
        "fact": "coins",
        "op": "add",
        "value": 1
       }
      ]
     },
     {
      "action": "move_to",
      "context": "coin_a_to_coin_c",
      "cost": 1.0,
      "preconditions": [
       {
        "fact": "atCoinA",
        "value": true
       },
       {
        "fact": "coinCCollected",
        "value": false
       },
       {
        "fact": "coinCAvailable",
        "value": true
       }
      ],
      "effects": [
       {
        "fact": "atCoinA",
        "value": false
       },
       {
        "fact": "atCoinC",
        "value": true
       },
       {
        "fact": "coinCCollected",
        "value": true
       }
      ],
      "numericEffects": [
       {
        "fact": "coins",
        "op": "add",
        "value": 1
       }
      ]
     },
     {
      "action": "move_to",
      "context": "coin_a_to_access_key_shop",
      "cost": 1.0,
      "preconditions": [
       {
        "fact": "atCoinA",
        "value": true
       },
       {
        "fact": "hasAccessKey",
        "value": false
       }
      ],
      "effects": [
       {
        "fact": "atCoinA",
        "value": false
       },
       {
        "fact": "atAccessKeyShop",
        "value": true
       }
      ],
      "numericPreconditions": [
       {
        "fact": "coins",
        "op": ">=",
        "value": 2
       }
      ]
     },
     {
      "action": "move_to",
      "context": "coin_b_to_coin_a",
      "cost": 1.0,
      "preconditions": [
       {
        "fact": "atCoinB",
        "value": true
       },
       {
        "fact": "coinACollected",
        "value": false
       },
       {
        "fact": "coinAAvailable",
        "value": true
       }
      ],
      "effects": [
       {
        "fact": "atCoinB",
        "value": false
       },
       {
        "fact": "atCoinA",
        "value": true
       },
       {
        "fact": "coinACollected",
        "value": true
       }
      ],
      "numericEffects": [
       {
        "fact": "coins",
        "op": "add",
        "value": 1
       }
      ]
     },
     {
      "action": "move_to",
      "context": "coin_b_to_coin_c",
      "cost": 1.0,
      "preconditions": [
       {
        "fact": "atCoinB",
        "value": true
       },
       {
        "fact": "coinCCollected",
        "value": false
       },
       {
        "fact": "coinCAvailable",
        "value": true
       }
      ],
      "effects": [
       {
        "fact": "atCoinB",
        "value": false
       },
       {
        "fact": "atCoinC",
        "value": true
       },
       {
        "fact": "coinCCollected",
        "value": true
       }
      ],
      "numericEffects": [
       {
        "fact": "coins",
        "op": "add",
        "value": 1
       }
      ]
     },
     {
      "action": "move_to",
      "context": "coin_b_to_access_key_shop",
      "cost": 1.0,
      "preconditions": [
       {
        "fact": "atCoinB",
        "value": true
       },
       {
        "fact": "hasAccessKey",
        "value": false
       }
      ],
      "effects": [
       {
        "fact": "atCoinB",
        "value": false
       },
       {
        "fact": "atAccessKeyShop",
        "value": true
       }
      ],
      "numericPreconditions": [
       {
        "fact": "coins",
        "op": ">=",
        "value": 2
       }
      ]
     },
     {
      "action": "move_to",
      "context": "coin_c_to_coin_a",
      "cost": 1.0,
      "preconditions": [
       {
        "fact": "atCoinC",
        "value": true
       },
       {
        "fact": "coinACollected",
        "value": false
       },
       {
        "fact": "coinAAvailable",
        "value": true
       }
      ],
      "effects": [
       {
        "fact": "atCoinC",
        "value": false
       },
       {
        "fact": "atCoinA",
        "value": true
       },
       {
        "fact": "coinACollected",
        "value": true
       }
      ],
      "numericEffects": [
       {
        "fact": "coins",
        "op": "add",
        "value": 1
       }
      ]
     },
     {
      "action": "move_to",
      "context": "coin_c_to_coin_b",
      "cost": 1.0,
      "preconditions": [
       {
        "fact": "atCoinC",
        "value": true
       },
       {
        "fact": "coinBCollected",
        "value": false
       },
       {
        "fact": "coinBAvailable",
        "value": true
       }
      ],
      "effects": [
       {
        "fact": "atCoinC",
        "value": false
       },
       {
        "fact": "atCoinB",
        "value": true
       },
       {
        "fact": "coinBCollected",
        "value": true
       }
      ],
      "numericEffects": [
       {
        "fact": "coins",
        "op": "add",
        "value": 1
       }
      ]
     },
     {
      "action": "move_to",
      "context": "coin_c_to_access_key_shop",
      "cost": 1.0,
      "preconditions": [
       {
        "fact": "atCoinC",
        "value": true
       },
       {
        "fact": "hasAccessKey",
        "value": false
       }
      ],
      "effects": [
       {
        "fact": "atCoinC",
        "value": false
       },
       {
        "fact": "atAccessKeyShop",
        "value": true
       }
      ],
      "numericPreconditions": [
       {
        "fact": "coins",
        "op": ">=",
        "value": 2
       }
      ]
     },
     {
      "action": "move_to",
      "context": "access_key_shop_to_goal",
      "cost": 1.0,
      "preconditions": [
       {
        "fact": "atAccessKeyShop",
        "value": true
       },
       {
        "fact": "hasAccessKey",
        "value": true
       },
       {
        "fact": "atDestination",
        "value": false
       }
      ],
      "effects": [
       {
        "fact": "atAccessKeyShop",
        "value": false
       },
       {
        "fact": "atGoal",
        "value": true
       },
       {
        "fact": "atDestination",
        "value": true
       }
      ]
     },
     {
      "action": "purchase",
      "context": "buy_key",
      "cost": 1.0,
      "preconditions": [
       {
        "fact": "hasAccessKey",
        "value": false
       },
       {
        "fact": "atAccessKeyShop",
        "value": true
       }
      ],
      "numericPreconditions": [
       {
        "fact": "coins",
        "op": ">=",
        "value": 2
       }
      ],
      "effects": [
       {
        "fact": "hasAccessKey",
        "value": true
       }
      ],
      "numericEffects": [
       {
        "fact": "coins",
        "op": "add",
        "value": -2
       }
      ]
     }
    ]
   },
   "routeCosts": {
    "start_to_coin_a": 6.715832,
    "start_to_coin_b": 6.091174,
    "start_to_coin_c": 7.816802,
    "coin_a_to_coin_b": 12.649111,
    "coin_a_to_coin_c": 9.055385,
    "coin_a_to_access_key_shop": 7.211103,
    "coin_b_to_coin_a": 12.649111,
    "coin_b_to_coin_c": 12.083046,
    "coin_b_to_access_key_shop": 10.0,
    "coin_c_to_coin_a": 9.055385,
    "coin_c_to_coin_b": 12.083046,
    "coin_c_to_access_key_shop": 13.928388,
    "access_key_shop_to_goal": 17.002144
   },
   "presets": {
    "start": {
     "title": "Старт сценария",
     "state": {
      "atStart": true,
      "coinAAvailable": true,
      "coinBAvailable": true,
      "coinCAvailable": true
     }
    },
    "replan": {
     "title": "У монеты C, монета A исчезла",
     "state": {
      "atCoinC": true,
      "coinCCollected": true,
      "coins": 1,
      "coinBAvailable": true
     }
    },
    "broke": {
     "title": "Нет доступных монет",
     "state": {
      "atCoinC": true,
      "coinCCollected": true,
      "coins": 1
     }
    }
   }
  },
  "target_recovery": {
   "title": "Target Recovery",
   "goap": {
    "id": "target_recovery",
    "facts": [
     {
      "name": "atDestination",
      "type": "bool"
     },
     {
      "name": "goalAvailable",
      "type": "bool"
     }
    ],
    "goals": [
     {
      "name": "reachDestination",
      "score": 1,
      "facts": [
       {
        "fact": "atDestination",
        "value": true
       }
      ]
     }
    ],
    "actions": [
     {
      "action": "move_to",
      "context": "destination",
      "cost": 1,
      "preconditions": [
       {
        "fact": "goalAvailable",
        "value": true
       }
      ],
      "effects": [
       {
        "fact": "atDestination",
        "value": true
       }
      ],
      "continuationConditions": [
       {
        "fact": "goalAvailable",
        "value": true
       }
      ]
     }
    ]
   },
   "routeCosts": {},
   "presets": {
    "available": {
     "title": "Цель доступна",
     "state": {
      "goalAvailable": true
     }
    },
    "missing": {
     "title": "Цель временно недоступна",
     "state": {
      "goalAvailable": false
     }
    }
   }
  },
  "marker_visit": {
   "title": "Marker Visit",
   "goap": {
    "id": "marker_visit",
    "facts": [
     {
      "name": "markerReached",
      "type": "bool"
     },
     {
      "name": "markerPresent",
      "type": "bool"
     }
    ],
    "goals": [
     {
      "name": "visitMarker",
      "score": 1,
      "facts": [
       {
        "fact": "markerReached",
        "value": true
       }
      ]
     }
    ],
    "actions": [
     {
      "action": "move_to",
      "context": "visitMarker",
      "cost": 2,
      "preconditions": [
       {
        "fact": "markerPresent",
        "value": true
       }
      ],
      "effects": [
       {
        "fact": "markerReached",
        "value": true
       }
      ],
      "continuationConditions": [
       {
        "fact": "markerPresent",
        "value": true
       }
      ]
     }
    ]
   },
   "routeCosts": {},
   "presets": {
    "present": {
     "title": "Маркер есть",
     "state": {
      "markerPresent": true
     }
    },
    "reached": {
     "title": "Маркер уже достигнут",
     "state": {
      "markerPresent": true,
      "markerReached": true
     }
    }
   }
  }
 }
};
