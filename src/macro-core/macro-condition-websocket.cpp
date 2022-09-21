#include "macro-condition-edit.hpp"
#include "macro-condition-websocket.hpp"
#include "utility.hpp"
#include "advanced-scene-switcher.hpp"

#include <regex>

const std::string MacroConditionWebsocket::id = "websocket";

bool MacroConditionWebsocket::_registered = MacroConditionFactory::Register(
	MacroConditionWebsocket::id,
	{MacroConditionWebsocket::Create, MacroConditionWebsocketEdit::Create,
	 "AdvSceneSwitcher.condition.websocket"});

static std::map<MacroConditionWebsocket::Type, std::string> conditionTypes = {
	{MacroConditionWebsocket::Type::REQUEST,
	 "AdvSceneSwitcher.condition.websocket.type.request"},
	{MacroConditionWebsocket::Type::EVENT,
	 "AdvSceneSwitcher.condition.websocket.type.event"},
};

static bool matchRegex(const RegexConfig &conf, const std::string &msg,
		       const std::string &expr)
{
	auto regex = conf.GetRegularExpression(expr);
	if (!regex.isValid()) {
		return false;
	}
	auto match = regex.match(QString::fromStdString(msg));
	return match.hasMatch();
}

bool MacroConditionWebsocket::CheckCondition()
{
	const std::vector<std::string> *messages = nullptr;
	switch (_type) {
	case MacroConditionWebsocket::Type::REQUEST:
		messages = &switcher->websocketMessages;
		break;
	case MacroConditionWebsocket::Type::EVENT: {
		auto connection = GetConnectionByName(_connection);
		if (!connection) {
			return false;
		}
		messages = &connection->Events();
		break;
	}
	default:
		break;
	}

	if (!messages) {
		return false;
	}

	for (const auto &msg : *messages) {
		if (_regex.Enabled()) {
			if (matchRegex(_regex, msg, _message)) {
				return true;
			}
		} else {
			if (msg == _message) {
				return true;
			}
		}
	}
	return false;
}

bool MacroConditionWebsocket::Save(obs_data_t *obj)
{
	MacroCondition::Save(obj);
	obs_data_set_int(obj, "type", static_cast<int>(_type));
	obs_data_set_string(obj, "message", _message.c_str());
	_regex.Save(obj);
	obs_data_set_string(obj, "connection", _connection.c_str());
	return true;
}

bool MacroConditionWebsocket::Load(obs_data_t *obj)
{
	MacroCondition::Load(obj);
	_type = static_cast<Type>(obs_data_get_int(obj, "type"));
	_message = obs_data_get_string(obj, "message");
	_regex.Load(obj);
	// TOOD: remove in future version
	if (obs_data_has_user_value(obj, "useRegex")) {
		_regex.CreateBackwardsCompatibleRegex(
			obs_data_get_bool(obj, "useRegex"), false);
	}
	_connection = obs_data_get_string(obj, "connection");
	return true;
}

std::string MacroConditionWebsocket::GetShortDesc()
{
	if (_type == Type::REQUEST) {
		return "";
	}
	return _connection;
}

static inline void populateConditionSelection(QComboBox *list)
{
	for (auto entry : conditionTypes) {
		list->addItem(obs_module_text(entry.second.c_str()));
	}
}

MacroConditionWebsocketEdit::MacroConditionWebsocketEdit(
	QWidget *parent, std::shared_ptr<MacroConditionWebsocket> entryData)
	: QWidget(parent),
	  _conditions(new QComboBox(this)),
	  _message(new ResizingPlainTextEdit(this)),
	  _regex(new RegexConfigWidget(parent)),
	  _connection(new ConnectionSelection(this)),
	  _editLayout(new QHBoxLayout())
{
	populateConditionSelection(_conditions);

	QWidget::connect(_conditions, SIGNAL(currentIndexChanged(int)), this,
			 SLOT(ConditionChanged(int)));
	QWidget::connect(_message, SIGNAL(textChanged()), this,
			 SLOT(MessageChanged()));
	QWidget::connect(_regex, SIGNAL(RegexConfigChanged(RegexConfig)), this,
			 SLOT(RegexChanged(RegexConfig)));
	QWidget::connect(_connection, SIGNAL(SelectionChanged(const QString &)),
			 this,
			 SLOT(ConnectionSelectionChanged(const QString &)));

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(_editLayout);
	mainLayout->addWidget(_message);
	mainLayout->addWidget(_regex);
	setLayout(mainLayout);

	_entryData = entryData;
	UpdateEntryData();
	_loading = false;
}

void MacroConditionWebsocketEdit::SetupRequestEdit()
{
	_editLayout->removeWidget(_conditions);
	_editLayout->removeWidget(_connection);
	clearLayout(_editLayout);
	std::unordered_map<std::string, QWidget *> widgetPlaceholders = {
		{"{{type}}", _conditions},
		{"{{connection}}", _connection},
	};
	placeWidgets(
		obs_module_text(
			"AdvSceneSwitcher.condition.websocket.entry.request"),
		_editLayout, widgetPlaceholders);
	_connection->hide();
}

void MacroConditionWebsocketEdit::SetupEventEdit()
{
	_editLayout->removeWidget(_conditions);
	_editLayout->removeWidget(_connection);
	clearLayout(_editLayout);
	std::unordered_map<std::string, QWidget *> widgetPlaceholders = {
		{"{{type}}", _conditions},
		{"{{connection}}", _connection},
	};
	placeWidgets(
		obs_module_text(
			"AdvSceneSwitcher.condition.websocket.entry.event"),
		_editLayout, widgetPlaceholders);
	_connection->show();
}

void MacroConditionWebsocketEdit::UpdateEntryData()
{
	if (!_entryData) {
		return;
	}

	_conditions->setCurrentIndex(static_cast<int>(_entryData->_type));
	_message->setPlainText(QString::fromStdString(_entryData->_message));
	_regex->SetRegexConfig(_entryData->_regex);
	_connection->SetConnection(_entryData->_connection);

	if (_entryData->_type == MacroConditionWebsocket::Type::REQUEST) {
		SetupRequestEdit();
	} else {
		SetupEventEdit();
	}

	adjustSize();
	updateGeometry();
}

void MacroConditionWebsocketEdit::ConditionChanged(int index)
{
	if (_loading || !_entryData) {
		return;
	}

	std::lock_guard<std::mutex> lock(switcher->m);
	_entryData->_type = static_cast<MacroConditionWebsocket::Type>(index);
	if (_entryData->_type == MacroConditionWebsocket::Type::REQUEST) {
		SetupRequestEdit();
	} else {
		SetupEventEdit();
	}
	emit HeaderInfoChanged(
		QString::fromStdString(_entryData->GetShortDesc()));
}

void MacroConditionWebsocketEdit::MessageChanged()
{
	if (_loading || !_entryData) {
		return;
	}

	std::lock_guard<std::mutex> lock(switcher->m);
	_entryData->_message = _message->toPlainText().toUtf8().constData();

	adjustSize();
	updateGeometry();
}

void MacroConditionWebsocketEdit::ConnectionSelectionChanged(
	const QString &connection)
{
	if (_loading || !_entryData) {
		return;
	}

	std::lock_guard<std::mutex> lock(switcher->m);
	_entryData->_connection = connection.toStdString();
	emit(HeaderInfoChanged(connection));
}

void MacroConditionWebsocketEdit::RegexChanged(RegexConfig conf)
{
	if (_loading || !_entryData) {
		return;
	}

	std::lock_guard<std::mutex> lock(switcher->m);
	_entryData->_regex = conf;

	adjustSize();
	updateGeometry();
}