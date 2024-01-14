print("loading api server script")

local app = require("milua")

app.add_handler("GET", "/",
    function()
        return "<h1>Patchwerk Radio!</h1>", {
            ["Content-Type"] = "text/html"
        }
    end
)

app.add_handler("GET", "/health",
    function()
        return "OK", {
            ["Content-Type"] = "text/plain"
        }
    end
)

-- api_config passed in from C
print(" INFO: [API Server] running at " .. api_config.HOST .. ":" .. api_config.PORT)

app.start(api_config)
