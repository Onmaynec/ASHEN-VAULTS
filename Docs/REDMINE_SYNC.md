# ASHEN VAULTS — соответствие GitHub Issues и Redmine

Основной список задач проекта сейчас ведётся в [GitHub Issues](https://github.com/Onmaynec/ASHEN-VAULTS/issues). Этот файл содержит перечень работ, который можно использовать для синхронизации с Redmine.

**Синхронизация с Redmine не выполнена:** адрес сервера и подключение к его API пока не предоставлены. Коды ниже — внутренние названия разделов, не номера существующих задач Redmine. Перед переносом необходимо проверить, нет ли таких задач в самом Redmine.

| Направление | GitHub Issues | Состояние |
| --- | --- | --- |
| Windows-сборка и упаковка | [#1](https://github.com/Onmaynec/ASHEN-VAULTS/issues/1), [#5](https://github.com/Onmaynec/ASHEN-VAULTS/issues/5), [#22](https://github.com/Onmaynec/ASHEN-VAULTS/issues/22) | Не подтверждены UE4.27 сборка, запуск и EXE |
| Генерация и бой | [#2](https://github.com/Onmaynec/ASHEN-VAULTS/issues/2), [#4](https://github.com/Onmaynec/ASHEN-VAULTS/issues/4), [#8](https://github.com/Onmaynec/ASHEN-VAULTS/issues/8), [#19](https://github.com/Onmaynec/ASHEN-VAULTS/issues/19) | Нужны игровые проверки цикла 1–20 этажей |
| Сохранения и прогресс | [#12](https://github.com/Onmaynec/ASHEN-VAULTS/issues/12), [#13](https://github.com/Onmaynec/ASHEN-VAULTS/issues/13) | Есть контрольные точки; развитие города и миграция требуют работы |
| Графика и интерфейс | [#6](https://github.com/Onmaynec/ASHEN-VAULTS/issues/6), [#7](https://github.com/Onmaynec/ASHEN-VAULTS/issues/7), [#10](https://github.com/Onmaynec/ASHEN-VAULTS/issues/10), [#15](https://github.com/Onmaynec/ASHEN-VAULTS/issues/15), [#16](https://github.com/Onmaynec/ASHEN-VAULTS/issues/16), [#17](https://github.com/Onmaynec/ASHEN-VAULTS/issues/17) | Частичный Canvas/UMG-рендер; финальный арт и интеграция не завершены |
| Производительность | [#3](https://github.com/Onmaynec/ASHEN-VAULTS/issues/3), [#18](https://github.com/Onmaynec/ASHEN-VAULTS/issues/18) | Есть изменения таймера, нет игровых замеров |
| Кооператив и звук | [#14](https://github.com/Onmaynec/ASHEN-VAULTS/issues/14), [#11](https://github.com/Onmaynec/ASHEN-VAULTS/issues/11) | Запланировано |

При подключении Redmine следует сопоставить задачи по содержанию, сохранить их ссылки друг на друга и переносить **только фактический статус работы**. Закрытые PR или наличие кода сами по себе не подтверждают, что дефект исправлен или игра прошла тестирование.

## Изменения v1.2.0

- [PR #28](https://github.com/Onmaynec/ASHEN-VAULTS/pull/28) — четыре боевых профиля комнат и исправления Арканиста.
- [Issue #29](https://github.com/Onmaynec/ASHEN-VAULTS/issues/29) — неправильная привязка двух способностей Арканиста. Исправление находится в v1.2.0; игровая проверка остаётся в #22.
- [Issue #30](https://github.com/Onmaynec/ASHEN-VAULTS/issues/30) — жизненный цикл контрольной точки после смерти и запуска новой экспедиции. Проверка на Windows/UE4.27 обязательна перед закрытием.
- [Issue #8](https://github.com/Onmaynec/ASHEN-VAULTS/issues/8) — дополнительно проверить плотность новых типов комнат и их проходимость.
- [Issue #12](https://github.com/Onmaynec/ASHEN-VAULTS/issues/12) — проверить, что сохранения не позволяют продолжить завершённый забег, но не теряют рекорды.

В Redmine эти изменения автоматически не отправлялись. Таблица предназначена только для ручного сопоставления задач при подключении сервера.

## Изменения v1.3.0

- [PR #31](https://github.com/Onmaynec/ASHEN-VAULTS/pull/31) — новые схемы геометрии, тактические спавны, события подкреплений и 12 модификаторов врагов.
- [Issue #32](https://github.com/Onmaynec/ASHEN-VAULTS/issues/32) — проверка четырёх событий подкрепления, состояний SaveGame и блокировки выхода в UE4.27.
- [Issue #33](https://github.com/Onmaynec/ASHEN-VAULTS/issues/33) — игровой баланс модификаторов врагов, аур, регенерации, статусов, наград и производительности.
- [Issue #34](https://github.com/Onmaynec/ASHEN-VAULTS/issues/34) — проходимость восьми новых схем комнат и безопасность расстановки врагов.
- [Issue #22](https://github.com/Onmaynec/ASHEN-VAULTS/issues/22) — проверка компиляции UE4.27/Win64 и всего игрового цикла текущей версии.

Новые записи — локальные ссылки на GitHub, **не идентификаторы Redmine**. Сведения не отправлялись в Redmine автоматически.
