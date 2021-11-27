#pragma once
#include "macro-action-edit.hpp"
#include "macro-selection.hpp"

#include <QPushButton>
#include <QListWidget>
#include <QDialog>
#include <unordered_map>

class MacroActionRandom : public MultiMacroRefAction {
public:
	bool PerformAction();
	void LogAction();
	bool Save(obs_data_t *obj);
	bool Load(obs_data_t *obj);
	std::string GetId() { return id; };
	static std::shared_ptr<MacroAction> Create()
	{
		return std::make_shared<MacroActionRandom>();
	}

	// TODO: add weights to each macro ...
	// std::unordered_map<MacroRef, int> _weights;

private:
	MacroRef lastRandomMacro;
	static bool _registered;
	static const std::string id;
};

class MacroDialog : public QDialog {
	Q_OBJECT

public:
	MacroDialog(QWidget *parent);
	static bool AskForMacro(QWidget *parent, std::string &macroName);

private:
	MacroSelection *_macroSelection;
};

class MacroActionRandomEdit : public QWidget {
	Q_OBJECT

public:
	MacroActionRandomEdit(
		QWidget *parent,
		std::shared_ptr<MacroActionRandom> entryData = nullptr);
	void UpdateEntryData();
	static QWidget *Create(QWidget *parent,
			       std::shared_ptr<MacroAction> action)
	{
		return new MacroActionRandomEdit(
			parent,
			std::dynamic_pointer_cast<MacroActionRandom>(action));
	}

private slots:
	void MacroRemove(const QString &name);
	void MacroRename(const QString &oldName, const QString &newName);
	void AddMacro();
	void RemoveMacro();

protected:
	std::shared_ptr<MacroActionRandom> _entryData;

private:
	int FindEntry(const std::string &macro);
	void SetMacroListSize();

	QListWidget *_macroList;
	QPushButton *_add;
	QPushButton *_remove;
	bool _loading = true;
};