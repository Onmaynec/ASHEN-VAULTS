# ASHEN VAULTS — источники визуальных ресурсов

## Оригинальные ресурсы ASHEN VAULTS

В репозитории `Content/AVVisuals` лежат новые исходные PNG-атласы, созданные для этого проекта:

- `Backgrounds/MenuBackdrop.png` — авторский процедурно скомпонованный пиксельный фон главного меню.
- `Tiles/CryptAtlas.png`, `SporeAtlas.png`, `FoundryAtlas.png`, `BastionAtlas.png` — по 28 изображений пола/стен на каждый биом.
- `Characters/HeroAtlas.png` — четыре героя, 32 кадра.
- `Enemies/EnemyAtlas.png` — 57 видов врагов, 114 кадров.
- `Bosses/BossAtlas.png` — четыре главных босса, 24 кадра.
- `Icons/EquipmentAtlas.png` — 42 иконки оружия, доспехов и аксессуаров.

Эти ресурсы разработаны специально для ASHEN VAULTS. Не являются копиями спрайтов Pocket Rogues.

## Kenney — Tiny Dungeon (CC0 1.0 Universal)

- Создатель: **Kenney**.
- Официальный источник: https://kenney.nl/assets/tiny-dungeon
- Лицензия: **CC0 1.0 Universal**; бесплатное личное/коммерческое использование, разрешены изменения.
- Стабильное зеркало архива: https://github.com/bill2432/kenney-2d-mirror (используется только SHA-проверенная копия).
- Контрольная SHA256: `c109438ab06f65fd80f9b2686a4cf9c7c11dc64444b47333ec71d602f8bb5fc7`.
- Ресурс `Content/AVVisuals/Kenney/TinyDungeonDecor.png` получен из четырёх спрайтов пакета с авторским перекрашиванием под каждый биом; используется для декора, при отсутствии есть fallback.
- Лицензия и provenance продублированы в `Content/AVVisuals/Kenney/SOURCE_CC0.txt`.

## Бесплатные компоненты движка

- Unreal Engine 4.27: Paper2D, UMG, Niagara, ImageWrapper, Slate, встроенные редакторские Python-скрипты.
- Текстовые материалы в `Shaders` и скрипты их создания в `Tools/Unreal` содержатся в репозитории.
- Не загружайте сторонние ресурсы без проверки исходной лицензии. В частности, не добавляйте арты Pocket Rogues.
