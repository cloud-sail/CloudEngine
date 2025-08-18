#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"


//-----------------------------------------------------------------------------------------------
DevConsole* g_theDevConsole = nullptr;

//-----------------------------------------------------------------------------------------------
const Rgba8 DevConsole::ERROR = Rgba8(255, 0, 0);
const Rgba8 DevConsole::WARNING = Rgba8(255, 128, 50);
const Rgba8 DevConsole::INFO_MAJOR = Rgba8(50, 150, 255);
const Rgba8 DevConsole::INFO_MINOR = Rgba8(150, 200, 150);
const Rgba8 DevConsole::INFO_TEXT = Rgba8(200, 200, 200);
const Rgba8 DevConsole::INFO_INSERTION_POINT = Rgba8(255, 255, 255);
const Rgba8 DevConsole::INFO_ENTERED_TEXT = Rgba8(255, 0, 255);

STATIC bool DevConsole::Event_KeyPressed(EventArgs& args)
{
	if (g_theDevConsole == nullptr)
	{
		return false;
	}

	unsigned char keyCode = (unsigned char)args.GetValue("KeyCode", -1);

	if (keyCode == KEYCODE_TILDE)
	{
		g_theDevConsole->ToggleMode(DevConsoleMode::OPEN_FULL);
		ResetInsertionPointTimer();
		return true;
	}

	if (g_theDevConsole->GetMode() == DevConsoleMode::HIDDEN)
	{
		return false; // pass KeyPressed event to the input system
	}

	if (keyCode == KEYCODE_ESCAPE)
	{
		ResetInsertionPointTimer();
		if (g_theDevConsole->m_inputText.empty())
		{
			g_theDevConsole->SetMode(DevConsoleMode::HIDDEN);
		}
		else
		{
			g_theDevConsole->m_inputText.clear();
			g_theDevConsole->m_insertionPointPosition = 0;
		}
		return true;
	}
	if (keyCode == KEYCODE_ENTER)
	{
		ResetInsertionPointTimer();
		if (g_theDevConsole->m_inputText.empty())
		{
			g_theDevConsole->SetMode(DevConsoleMode::HIDDEN);
		}
		else
		{
			g_theDevConsole->Execute(g_theDevConsole->m_inputText);
			g_theDevConsole->m_inputText.clear();
			g_theDevConsole->m_insertionPointPosition = 0;
			g_theDevConsole->m_historyIndex = -1;
		}
		return true;
	}
	if (keyCode == KEYCODE_LEFT)
	{
		ResetInsertionPointTimer();
		if (g_theDevConsole->m_insertionPointPosition > 0)
		{
			g_theDevConsole->m_insertionPointPosition--;
		}
		return true;
	}
	if (keyCode == KEYCODE_RIGHT)
	{
		ResetInsertionPointTimer();
		if (g_theDevConsole->m_insertionPointPosition < (int)g_theDevConsole->m_inputText.length())
		{
			g_theDevConsole->m_insertionPointPosition++;
		}
		return true;
	}
	if (keyCode == KEYCODE_HOME)
	{
		ResetInsertionPointTimer();
		g_theDevConsole->m_insertionPointPosition = 0;
		return true;
	}
	if (keyCode == KEYCODE_END)
	{
		ResetInsertionPointTimer();
		g_theDevConsole->m_insertionPointPosition = (int)g_theDevConsole->m_inputText.length();
		return true;
	}
	if (keyCode == KEYCODE_DELETE)
	{
		ResetInsertionPointTimer();
		g_theDevConsole->m_inputText.erase(g_theDevConsole->m_insertionPointPosition, 1);
		return true;
	}
	if (keyCode == KEYCODE_BACKSPACE)
	{
		ResetInsertionPointTimer();
		if (g_theDevConsole->m_insertionPointPosition > 0)
		{
			g_theDevConsole->m_inputText.erase(g_theDevConsole->m_insertionPointPosition - 1, 1);
			g_theDevConsole->m_insertionPointPosition--;
		}
		return true;
	}
	if (keyCode == KEYCODE_UP)
	{
		ResetInsertionPointTimer();
		if (g_theDevConsole->m_commandHistory.IsEmpty())
		{
			return true;
		}

		if (g_theDevConsole->m_historyIndex < (g_theDevConsole->m_commandHistory.GetSize() - 1))
		{
			g_theDevConsole->m_historyIndex++;
		}
		g_theDevConsole->m_inputText = g_theDevConsole->m_commandHistory.GetHistoryData(g_theDevConsole->m_historyIndex);
		g_theDevConsole->m_insertionPointPosition = (int) g_theDevConsole->m_inputText.length();

		return true;
	}
	if (keyCode == KEYCODE_DOWN)
	{
		ResetInsertionPointTimer();
		if (g_theDevConsole->m_commandHistory.IsEmpty())
		{
			return true;
		}

		if (g_theDevConsole->m_historyIndex >= 0)
		{
			g_theDevConsole->m_historyIndex--;
		}
		g_theDevConsole->m_inputText = g_theDevConsole->m_commandHistory.GetHistoryData(g_theDevConsole->m_historyIndex);
		g_theDevConsole->m_insertionPointPosition = (int) g_theDevConsole->m_inputText.length();


		return true;
	}


	return true;
}

STATIC bool DevConsole::Event_CharInput(EventArgs& args)
{
	if (g_theDevConsole == nullptr)
	{
		return false;
	}

	if (g_theDevConsole->GetMode() == DevConsoleMode::HIDDEN)
	{
		return false;
	}

	unsigned char keyCode = (unsigned char)args.GetValue("KeyCode", -1);
	if (keyCode >= 32 && keyCode <= 126 && keyCode != '`' && keyCode != '~')
	{
		ResetInsertionPointTimer();

		g_theDevConsole->m_inputText.insert(g_theDevConsole->m_insertionPointPosition, 1, keyCode);
		g_theDevConsole->m_insertionPointPosition++;    
		return true;
	}

	return false;
}

STATIC bool DevConsole::Command_Clear(EventArgs& args)
{
	UNUSED(args);

	if (g_theDevConsole == nullptr)
	{
		return false;
	}

	g_theDevConsole->m_lines.clear();
	return true;
}

STATIC bool DevConsole::Command_Help(EventArgs& args)
{
	UNUSED(args);

	if (g_theDevConsole == nullptr)
	{
		return false;
	}
	Strings outCommandNames;
	g_theEventSystem->GetAllRegistedCommands(outCommandNames);
	g_theDevConsole->AddText(DevConsole::INFO_MAJOR, "Registered Commands");

	for (int commmandIndex = 0; commmandIndex < (int)outCommandNames.size(); ++commmandIndex)
	{
		g_theDevConsole->AddText(DevConsole::INFO_MINOR, outCommandNames[commmandIndex]);
	}

	return true;
}

STATIC bool DevConsole::Command_Test(EventArgs& args)
{
	float time = args.GetValue("elapsedTime", -100.f);
	g_theDevConsole->AddText(INFO_MINOR, Stringf("Test command received! time elapsed: %0.2f", time));
	//args.DebugPrintContents();
	return false; // Does not consume event; continue to call other subscribers’ callback functions
}

void DevConsole::Render_OpenFull(AABB2 const& bounds, Renderer& renderer, BitmapFont& font, float fontAspectScale /*= 1.f*/) const
{
	std::vector<Vertex_PCU> verts;

	float cellHeight = bounds.GetDimensions().y / m_config.m_linesOnScreen;


	// Input Text and Insertion point - First Row
	AABB2 inputTextBox = AABB2(bounds.m_mins.x, bounds.m_mins.y, bounds.m_maxs.x, bounds.m_mins.y + cellHeight);
	font.AddVertsForTextInBox2D(verts, m_inputText, inputTextBox, cellHeight, DevConsole::INFO_TEXT, fontAspectScale, Vec2(0.f, 0.5f));

	// Lines in DevConsole
	int currentRow = 1;

	for (int lineIndex = static_cast<int>(m_lines.size() - 1); lineIndex >= 0; --lineIndex)
	{
		if (static_cast<float>(currentRow) > m_config.m_linesOnScreen)
		{
			break;
		}

		float minY = bounds.m_mins.y + static_cast<float>(currentRow) * cellHeight;
		AABB2 box = AABB2(bounds.m_mins.x, minY, bounds.m_maxs.x, minY + cellHeight);

		DevConsoleLine const& line = m_lines[lineIndex];
		font.AddVertsForTextInBox2D(verts, line.m_text, box, cellHeight, line.m_color, fontAspectScale, Vec2(0.f, 0.5f));
		
		currentRow++;
	}

#ifdef ENGINE_RENDER_D3D11
	renderer.BindTexture(&font.GetTexture());
	renderer.SetSamplerMode(SamplerMode::POINT_CLAMP);
#endif // ENGINE_RENDER_D3D11

	//No need to Set Model Constants? it is set in BeginCamera
#ifdef ENGINE_RENDER_D3D12
	// resource settings
	UnlitRenderResources resources;
	resources.diffuseTextureIndex = renderer.GetSrvIndexFromLoadedTexture(&font.GetTexture(), DefaultTexture::WhiteOpaque2D);
	resources.diffuseSamplerIndex = renderer.GetDefaultSamplerIndex(SamplerMode::POINT_CLAMP);
	resources.cameraConstantsIndex = renderer.GetCurrentCameraConstantsIndex();
	resources.modelConstantsIndex = renderer.GetCurrentModelConstantsIndex();

	renderer.SetGraphicsBindlessResources(sizeof(UnlitRenderResources), &resources);
#endif // ENGINE_RENDER_D3D12

	renderer.BindShader(nullptr);
	renderer.SetBlendMode(BlendMode::ALPHA);
	renderer.SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	renderer.SetDepthMode(DepthMode::DISABLED);
	renderer.SetRenderTargetFormats();
	renderer.DrawVertexArray(verts);

	if (m_insertionPointVisible)
	{
		std::vector<Vertex_PCU> insertionPointVerts;
		Vec2 insertionPointStartPos;
		float insertionPointLength = 0.f;
		font.GetInsertionPointForTextInBox2D(insertionPointLength, insertionPointStartPos, m_insertionPointPosition, m_inputText, inputTextBox, cellHeight,
			fontAspectScale, Vec2(0.f, 0.5f));
		constexpr float insertionPointAspectRatio = 0.15f;
		AddVertsForLineSegment2D(insertionPointVerts, insertionPointStartPos, insertionPointStartPos + Vec2(0.f, insertionPointLength), insertionPointAspectRatio * fontAspectScale * insertionPointLength, DevConsole::INFO_INSERTION_POINT);

#ifdef ENGINE_RENDER_D3D11
		renderer.BindTexture(nullptr);
		renderer.SetSamplerMode(SamplerMode::POINT_CLAMP);
#endif // ENGINE_RENDER_D3D11

		//No need to Set Model Constants? it is set in BeginCamera
#ifdef ENGINE_RENDER_D3D12
		// resource settings
		UnlitRenderResources insertionResources;
		insertionResources.diffuseTextureIndex = renderer.GetSrvIndexFromLoadedTexture(nullptr, DefaultTexture::WhiteOpaque2D);
		insertionResources.diffuseSamplerIndex = renderer.GetDefaultSamplerIndex(SamplerMode::POINT_CLAMP);
		insertionResources.cameraConstantsIndex = renderer.GetCurrentCameraConstantsIndex();
		insertionResources.modelConstantsIndex = renderer.GetCurrentModelConstantsIndex();

		renderer.SetGraphicsBindlessResources(sizeof(UnlitRenderResources), &insertionResources);
#endif // ENGINE_RENDER_D3D12

		renderer.BindShader(nullptr);
		renderer.SetBlendMode(BlendMode::ALPHA);
		renderer.SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		renderer.SetDepthMode(DepthMode::DISABLED);
		renderer.SetRenderTargetFormats();
		renderer.DrawVertexArray(insertionPointVerts);
	}
}

STATIC void DevConsole::ResetInsertionPointTimer()
{
	g_theDevConsole->m_insertionPointBlinkTimer->Start();
	g_theDevConsole->m_insertionPointVisible = true;
}

//-----------------------------------------------------------------------------------------------
DevConsole::DevConsole(DevConsoleConfig const& config)
	: m_config(config)
{
	m_fontFilePathWithoutExtension = "Data/Fonts/" + m_config.m_fontName;
	m_insertionPointBlinkTimer = new Timer(0.5f);
	m_commandHistory.Resize(m_config.m_maxCommandHistory);
}

DevConsole::~DevConsole()
{

}

void DevConsole::Startup()
{
	// Subscribe event
	g_theEventSystem->SubscribeEventCallbackFunction("CharInput", DevConsole::Event_CharInput);
	g_theEventSystem->SubscribeEventCallbackFunction("KeyPressed", DevConsole::Event_KeyPressed);
	g_theEventSystem->SubscribeEventCallbackFunction("clear", DevConsole::Command_Clear);
	g_theEventSystem->SubscribeEventCallbackFunction("help", DevConsole::Command_Help);

	AddText(DevConsole::INFO_MINOR, "Type help for a list of commands.");
}

void DevConsole::Shutdown()
{
	// Unsubscribe event
	g_theEventSystem->UnsubscribeEventCallbackFunction("CharInput", DevConsole::Event_CharInput);
	g_theEventSystem->UnsubscribeEventCallbackFunction("KeyPressed", DevConsole::Event_KeyPressed);
	g_theEventSystem->UnsubscribeEventCallbackFunction("clear", DevConsole::Command_Clear);
	g_theEventSystem->UnsubscribeEventCallbackFunction("help", DevConsole::Command_Help);
}

void DevConsole::BeginFrame()
{
	if (m_insertionPointBlinkTimer->DecrementPeriodIfElapsed())
	{
		m_insertionPointVisible = !m_insertionPointVisible;
	}
}

void DevConsole::EndFrame()
{

}

void DevConsole::Execute(std::string const& consoleCommandText, bool echoCommand /*= true*/)
{
	Strings commandLines = SplitStringOnDelimiter(consoleCommandText, '\n');

	for (size_t commandIndex = 0; commandIndex < commandLines.size(); ++commandIndex)
	{
		std::string const& commandLine = commandLines[commandIndex];
		Strings parts = SplitStringOnDelimiterAndDiscardEmpty(commandLine, ' ');
		if (parts.empty()) continue;

		DevConsoleCommand cmd;
		cmd.name = parts[0];


		// ToFix tier 1 solution: if '=' in value, if key = value, if key = " abc " 
		for (size_t i = 1; i < parts.size(); ++i) 
		{
			std::string const& arg = parts[i];
			size_t eqPos = arg.find('=');
			if (eqPos != std::string::npos) {
				std::string key = arg.substr(0, eqPos);
				std::string value = arg.substr(eqPos + 1);
				cmd.kwargs[key] = value;
			}
			else {
				cmd.args.push_back(arg);
			}
		}
		// cmd.args is positional/ordered arguments, not used now
		// cmd.kwargs is keyword arguments
		if (echoCommand)
		{
			Rgba8 color = INFO_ENTERED_TEXT;
			SaveLine(color, commandLine);
		}
		SaveHistoryCommandLine(commandLine);

		EventArgs args(cmd.kwargs);
		g_theEventSystem->FireEvent(cmd.name, args);
	}
}

void DevConsole::AddText(Rgba8 const& color, std::string const& text)
{
	Strings lines = SplitStringOnDelimiter(text, '\n');

	for (size_t index = 0; index < lines.size(); ++index)
	{
		SaveLine(color, lines[index]);
	}
}

void DevConsole::Render(AABB2 const& bounds, Renderer* rendererOverride /*= nullptr*/) const
{
	if (m_mode == DevConsoleMode::HIDDEN)
	{
		return;
	}
	Renderer* renderer = m_config.m_defaultRenderer;
	if (rendererOverride)
	{
		renderer = rendererOverride;
	}
	BitmapFont* font = renderer->CreateOrGetBitmapFont(m_fontFilePathWithoutExtension.c_str());

	// translucent black quad
	std::vector<Vertex_PCU> verts;
	AddVertsForAABB2D(verts, bounds, Rgba8(0, 0, 0, 128));

#ifdef ENGINE_RENDER_D3D11
	renderer->BindTexture(nullptr);
	renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
#endif // ENGINE_RENDER_D3D11

#ifdef ENGINE_RENDER_D3D12
	// resource settings
	UnlitRenderResources resources;
	resources.diffuseTextureIndex = renderer->GetSrvIndexFromLoadedTexture(nullptr, DefaultTexture::WhiteOpaque2D);
	resources.diffuseSamplerIndex = renderer->GetDefaultSamplerIndex(SamplerMode::POINT_CLAMP);
	resources.cameraConstantsIndex = renderer->GetCurrentCameraConstantsIndex();
	resources.modelConstantsIndex = renderer->GetCurrentModelConstantsIndex();

	renderer->SetGraphicsBindlessResources(sizeof(UnlitRenderResources), &resources);
#endif // ENGINE_RENDER_D3D12


	renderer->BindShader(nullptr);
	renderer->SetBlendMode(BlendMode::ALPHA);
	renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	renderer->SetDepthMode(DepthMode::DISABLED);
	renderer->SetRenderTargetFormats();

	renderer->DrawVertexArray(verts);

	switch (m_mode)
	{
	case DevConsoleMode::OPEN_FULL:
		Render_OpenFull(bounds, *renderer, *font, m_config.m_fontAspectScale);
		break;
	}
}

DevConsoleMode DevConsole::GetMode() const
{
	return m_mode;
}

void DevConsole::SetMode(DevConsoleMode mode)
{
	m_mode = mode;
}

void DevConsole::ToggleMode(DevConsoleMode mode)
{
	if (m_mode == mode)
	{
		m_mode = DevConsoleMode::HIDDEN;
	}
	else
	{
		m_mode = mode;
	}
}

void DevConsole::SaveLine(Rgba8 const& color, std::string const& lineText)
{
	DevConsoleLine devConsoleLine;
	devConsoleLine.m_color = color;
	devConsoleLine.m_text = lineText;
	m_lines.push_back(devConsoleLine);
}



void DevConsole::SaveHistoryCommandLine(std::string const& lineText)
{
	m_commandHistory.Enqueue(lineText);
}

//-----------------------------------------------------------------------------------------------
CircularQueue::CircularQueue(int capacity)
	: m_capacity(capacity)
{
	m_data.resize(m_capacity);
}

void CircularQueue::Resize(int capacity)
{
	m_start = 0;
	m_end = 0;
	m_capacity = capacity;
	m_count = 0;
	m_data.resize(m_capacity);
}

void CircularQueue::Enqueue(std::string const& value)
{
	if (isFull())
	{
		m_data[m_end] = value;
		m_end = (m_end + 1) % m_capacity;
		m_start = m_end;
	}
	else
	{
		m_data[m_end] = value;
		m_end = (m_end + 1) % m_capacity;
		m_count++;
	}
}

std::string CircularQueue::GetHistoryData(int historyIndex)
{
	if (IsEmpty())
	{
		return "";
	}
	if (historyIndex < 0 || historyIndex >= m_count)
	{
		return "";
	}
	int index = (m_end + m_capacity - 1 - historyIndex) % m_capacity;
	return m_data[index];
}

int CircularQueue::GetSize() const
{
	return m_count;
}

bool CircularQueue::isFull() const
{
	return m_count == m_capacity;
}

bool CircularQueue::IsEmpty() const
{
	return m_count == 0;
}
