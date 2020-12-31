#include "headers/advanced-scene-switcher.hpp"
#include "headers/utility.hpp"

bool SceneSwitcherEntry::initialized()
{
	return (usePreviousScene || WeakSourceValid(scene)) && transition;
}

bool SceneSwitcherEntry::valid()
{
	return !initialized() ||
	       ((usePreviousScene || WeakSourceValid(scene)) &&
		WeakSourceValid(transition));
}

void SceneSwitcherEntry::logMatch()
{
	const char *sceneName = previous_scene_name;
	if (!usePreviousScene) {
		obs_source_t *s = obs_weak_source_get_source(scene);
		sceneName = obs_source_get_name(s);
		obs_source_release(s);
	}
	blog(LOG_INFO, "match for '%s' - switch to scene '%s'", getType(),
	     sceneName);
}

SwitchWidget::SwitchWidget(SceneSwitcherEntry *s, bool usePreviousScene)
{
	scenes = new QComboBox();
	transitions = new QComboBox();

	// Depending on selected OBS theme some widgets might have a different
	// background color than the listwidget and might look out of place
	setStyleSheet("QLabel { background-color: transparent; }\
		       QSlider { background-color: transparent; }\
		       QCheckBox { background-color: transparent; }");

	QWidget::connect(scenes, SIGNAL(currentTextChanged(const QString &)),
			 this, SLOT(SceneChanged(const QString &)));
	QWidget::connect(transitions,
			 SIGNAL(currentTextChanged(const QString &)), this,
			 SLOT(TransitionChanged(const QString &)));

	AdvSceneSwitcher::populateSceneSelection(scenes, usePreviousScene);
	AdvSceneSwitcher::populateTransitionSelection(transitions);

	if (s) {
		if (s->usePreviousScene)
			scenes->setCurrentText(obs_module_text(
				"AdvSceneSwitcher.selectPreviousScene"));
		else
			scenes->setCurrentText(
				GetWeakSourceName(s->scene).c_str());
		transitions->setCurrentText(
			GetWeakSourceName(s->transition).c_str());
	}

	switchData = s;
}

SceneSwitcherEntry *SwitchWidget::getSwitchData()
{
	return switchData;
}

void SwitchWidget::setSwitchData(SceneSwitcherEntry *s)
{
	switchData = s;
}

void SwitchWidget::swapSwitchData(SwitchWidget *s1, SwitchWidget *s2)
{
	SceneSwitcherEntry *t = s1->getSwitchData();
	s1->setSwitchData(s2->getSwitchData());
	s2->setSwitchData(t);
}

bool isPreviousScene(const QString &text)
{
	return text.compare(obs_module_text(
		       "AdvSceneSwitcher.selectPreviousScene")) == 0;
}

void SwitchWidget::SceneChanged(const QString &text)
{
	if (loading || !switchData)
		return;
	std::lock_guard<std::mutex> lock(switcher->m);
	switchData->usePreviousScene = isPreviousScene(text);
	if (switchData->usePreviousScene)
		return;
	switchData->scene = GetWeakSourceByQString(text);
}

void SwitchWidget::TransitionChanged(const QString &text)
{
	if (loading || !switchData)
		return;
	std::lock_guard<std::mutex> lock(switcher->m);
	switchData->transition = GetWeakTransitionByQString(text);
}
