# Redmine — ASHEN VAULTS, перенос задач v0.9.0 / v1.0.0

**Статус:** подготовленная синхронизация. **В Redmine изменения ещё не отправлены**: в текущей сессии нет подключённого Redmine, адреса сервера и доступа к его API. Не использовать этот документ как подтверждение синхронизации.

## Задача AV-1.0-ART — Перерождение визуальной системы
- Статус: исходный код реализован, сборка UE4.27 не подтверждена.
- Реализовано: 4 тайлсета, 32 кадра героев, 114 кадров мобов, 24 кадра боссов, 42 иконки снаряжения, 16 классов декора, CC0 Kenney.
- Осталось: Paper2D TileMap/Flipbooks, проверка арта всех биомов, оформление антуража в игровом окне.

## Задача AV-1.0-UI — Главное меню и интерфейс
- Реализовано: UMG главное меню, выбор классов, Continue/New Run, настройки; Canvas панели инвентаря и дерева навыков с графикой.
- Осталось: полное UMG HUD, RU/EN локализация, DPI/масштабирование и usability QA.

## Задача AV-1.0-VFX — Материалы, свет, частицы
- Реализовано: Canvas свет/тьма, новые эффекты снарядов и попаданий, editor Python скрипт для четырёх Material графов, Niagara plugin enabled.
- Осталось: создать материальные .uasset в Unreal Editor, NiagaraSystem, проверить GPU, оптимизировать FPS.

## Задача AV-1.0-BUILD — Проверка сборки и выпуск Windows
- Статус: блокер.
- Осталось: UE4.27 UHT, Win64 компиляция, тест сохранений, копирование PNG Non-UFS, пакетирование EXE, smoke-run без Editor.

## Источник истины в GitHub

- PR v0.9.0: https://github.com/Onmaynec/ASHEN-VAULTS/pull/21
- Epic v1.0.0 QA: https://github.com/Onmaynec/ASHEN-VAULTS/issues/22
- Спрайты и Paper2D: https://github.com/Onmaynec/ASHEN-VAULTS/issues/7
- UI/UMG: https://github.com/Onmaynec/ASHEN-VAULTS/issues/10
- Свет и материалы: https://github.com/Onmaynec/ASHEN-VAULTS/issues/15
- VFX: https://github.com/Onmaynec/ASHEN-VAULTS/issues/16
- Сборка: https://github.com/Onmaynec/ASHEN-VAULTS/issues/1

После подключения Redmine можно обновить существующие задачи, не создавая дублей. Номера AV-1.0-* — локальные ключи документа, не настоящие ID Redmine.
