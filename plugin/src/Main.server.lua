local HttpService = game:GetService("HttpService")

local MarketplaceService = game:GetService("MarketplaceService")
local Selection = game:GetService("Selection")
local StudioService = game:GetService("StudioService")
local RunService = game:GetService("RunService")

local States = require(script.Parent.States)
local Settings = require(script.Parent.Settings)

local gameName

local success = pcall(function()
	gameName = MarketplaceService:GetProductInfo(game.PlaceId).Name
end)

if not success then
	gameName = game:GetFullName()
end

local currentState: number = States.Idle
local currentScript = ""
local lastActionTime = os.clock()

local function send(): ()
	pcall(function()
		HttpService:PostAsync("http://localhost:" .. Settings.Port, HttpService:JSONEncode({
			State = currentState;
			Script = currentScript;
			
			GameName = gameName;
		}), Enum.HttpContentType.ApplicationJson, false)
	end)
end

local function set(state: number, editingScript: string?): ()
	currentState = state
	currentScript = editingScript or ""
	
	lastActionTime = os.clock()
	
	if state ~= currentState or editingScript ~= currentScript then
		send()
	end
end


if not RunService:IsEdit() then
	if RunService:IsClient() then return end
	
	set(States.Playtesting)
	
	while task.wait(Settings.NoChangeSendInterval) do
		send()
	end
	
	return
end


local function activeScriptChanged()
	if StudioService.ActiveScript then
		set(States.Scripting, StudioService.ActiveScript:GetFullName())
	else
		set(States.Idle)
	end
end

activeScriptChanged()
StudioService:GetPropertyChangedSignal("ActiveScript"):Connect(activeScriptChanged)


Selection.SelectionChanged:Connect(function()
	local hasPart = false
	
	for i,v in Selection:Get() do
		if v:IsA("BaseScript") then return end
		if v:IsA("BasePart") then
			hasPart = true
			
			break
		end
	end
	
	if not hasPart then return end
	
	set(States.Building)
end)

task.spawn(function()
	while task.wait(Settings.IdleCheckInterval) do
		if os.clock() - lastActionTime >= Settings.IdleTimeout then
			set(States.Idle)
		end
	end
end)

while task.wait(Settings.NoChangeSendInterval) do
	send()
end
