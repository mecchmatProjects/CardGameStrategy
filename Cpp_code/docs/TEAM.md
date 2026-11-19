# Команда, ролі та план роботи

## Ролі

| | Учасник 1 | Учасник 2 | Учасник 3 |
|---|---|---|---|
| **Роль** | Ядро, оцінка рук, інфраструктура | Ігровий рушій та гра | Аналітика та штучний інтелект |
| **Модулі** | `core/`, `eval/`, `ui/Console`, CMake, CI | `game/`, `ui/ConsoleObserver`, `ui/HumanStrategy`, `ui/App`, `app/` | `analysis/`, `ai/`, `ui/AnalysisScreens` |
| **Графіка** | `gui/Render` (шрифти, карти, фішки, кнопки), raylib у CMake, `assets/` | `gui/GameBridge` (потоки), `gui/PokerApp` (стіл, анімації, панель дій, меню) | `gui/AnalysisPanel` (еквіті, аути, порада, префлоп-таблиця, ол-ін шанси), автогра |
| **Тести** | `test_card`, `test_evaluator` | `test_pots`, `test_game` | `test_analysis`, `test_bots` |
| **Документи** | `README.md`, `.clang-format`, CI | розділ про правила гри | `docs/STRATEGY.md` |
| **На захисті пояснює** | бітові маски в оцінювачі, як порівнюються руки, як малюються карти й шрифти з кирилицею | торгівля, мін. рейз, сайд-поти, Strategy/Observer, потоки й черга подій у GUI | еквіті, Монте-Карло, діапазони суперників, EV і fold equity, формула Чена, як аналітика вбудована в гру |

Кожен також **рев'юїть** пул-реквести інших (коментарі в PR — теж видима активність у репозиторії).

## Узгоджені інтерфейси (зробити першими)

Щоб працювати паралельно, у перші дні фіксуємо "контракти" між модулями:

1. **Учасник 1** — `Card`, `HoleCards`, `parseCards` (`include/poker/core/Card.hpp`).
2. **Учасник 2** — `Action`, `DecisionContext`, `Strategy`, `Street`, `Position` (`include/poker/game/`).
3. **Учасник 1** — сигнатура `evaluateHand(...)` і `HandValue` (реалізацію можна дописати пізніше).

Після цього Учасник 3 пише аналітику й ботів проти `DecisionContext`, не чекаючи рушія гри.

## Правила роботи з Git

Репозиторій курсу — [mecchmatProjects/CardGameStrategy](https://github.com/mecchmatProjects/CardGameStrategy),
наш код — лише в папці `Cpp_code/` (інші папки належать іншим мовам/командам, їх не чіпаємо).

```
main  ←──────────── PR наприкінці (перевіряє викладач)
  └── cpp-dev  ←──── PR від кожного учасника, рев'ю від іншого учасника
        ├── cpp-<учасник1>
        ├── cpp-<учасник2>
        └── cpp-<учасник3>
```

- `main` — гілка викладача, напряму туди нічого не пушимо.
- `cpp-dev` — спільна гілка команди; завжди компілюється й проходить тести (CI перевіряє кожен PR).
- Кожен працює у **своїй гілці** `cpp-<ім'я>`, створеній від `cpp-dev`.
- Готову частину відправляє через **Pull Request** у `cpp-dev`; змерджити його має **інший учасник**
  після перегляду (рев'ю).
- Перед новою роботою оновіть свою гілку: `git pull origin cpp-dev`.
- Коли все готово — один PR `cpp-dev → main` для викладача.
- **Не використовуйте "Squash and merge"** — він зливає всі коміти гілки в один.
  Обирайте **"Create a merge commit"**.
- Коміт = одна логічна зміна, що компілюється. Краще 3 маленьких коміти, ніж 1 великий.
- Повідомлення комітів — у форматі [Conventional Commits](https://www.conventionalcommits.org/):

  | Префікс | Коли |
    |---|---|
  | `feat(модуль):` | нова функціональність |
  | `fix(модуль):` | виправлення помилки |
  | `test(модуль):` | тести |
  | `refactor(модуль):` | зміна коду без зміни поведінки |
  | `docs:` | документація |
  | `build:` / `ci:` / `chore:` | CMake, CI, налаштування |

- Перед першим комітом кожен налаштовує своє ім'я, щоб авторство було видно:
  ```bash
  git config user.name "Ім'я Прізвище"
  git config user.email "пошта@прив'язана.до.github"
  ```
- Задачі зручно вести в **GitHub Issues / Projects**; у коміті можна писати `closes #12`.

### Як не ламати збірку

- Додаючи новий `.cpp`, у **тому ж коміті** додайте його в `CMakeLists.txt` свого модуля.
- Рядок `add_subdirectory(src/<модуль>)` у кореневому `CMakeLists.txt` додає власник модуля
  у своєму першому коміті з кодом цього модуля.
- `tests/CMakeLists.txt`: кожен додає свої тест-файли. Бібліотека для лінкування тестів
  змінюється в міру появи модулів: `poker_eval` → `poker_game` → `poker_ai`.

## План комітів

Порядок усередині кожного стовпця важливий. Тижні — орієнтовні.

### Учасник 1 — ядро, оцінка рук, інфраструктура

| # | Тиждень | Коміт |
|---|---|---|
| 1 | 1 | `chore: initial project structure with CMake and .gitignore` |
| 2 | 1 | `feat(core): add Suit and Rank enums and Card class` |
| 3 | 1 | `feat(core): parse cards from text (Card::fromString, parseCards)` |
| 4 | 1 | `feat(core): card bit masks and pretty output with suit symbols` |
| 5 | 1 | `feat(core): add Rng wrapper over mt19937_64` |
| 6 | 1 | `feat(core): add Deck with shuffle, deal and excluded cards` |
| 7 | 1 | `test: add minimal test framework and test runner` |
| 8 | 1 | `test(core): cover card parsing and deck` |
| 9 | 2 | `feat(eval): HandValue with packed score and category names` |
| 10 | 2 | `feat(eval): detect pairs, two pair, trips, full house, quads` |
| 11 | 2 | `feat(eval): detect straights (incl. wheel), flushes, straight flushes` |
| 12 | 2 | `feat(eval): human-readable hand description` |
| 13 | 2 | `test(eval): categories, kickers, best 5 of 7` |
| 14 | 3 | `feat(ui): console colors and UTF-8 setup for Windows` |
| 15 | 3 | `feat(ui): input helpers readLine, readInt, readCards` |
| 16 | 3 | `feat(ui): UTF-8 aware padding and progress bar` |
| 17 | 3 | `ci: build and test on Linux and Windows with GitHub Actions` |
| 18 | 4 | `chore: add .clang-format and format sources` |
| 19 | 4 | `docs: README with build instructions and architecture` |

### Учасник 2 — ігровий рушій та гра

| # | Тиждень | Коміт |
|---|---|---|
| 1 | 1 | `feat(game): Street and Position enums` |
| 2 | 1 | `feat(game): Action type with factory methods and toString` |
| 3 | 1 | `feat(game): DecisionContext and Strategy interface` |
| 4 | 1 | `feat(game): Player with stack, street bet and all-in state` |
| 5 | 2 | `feat(game): side pot calculation (buildPots)` |
| 6 | 2 | `test(game): side pot scenarios` |
| 7 | 2 | `feat(game): GameObserver events` |
| 8 | 2 | `feat(game): Game skeleton with seats, dealer button and blinds` |
| 9 | 2 | `feat(game): deal hole cards and board with burn cards` |
| 10 | 2 | `feat(game): betting round with raises and min-raise rule` |
| 11 | 3 | `feat(game): validate actions (free fold → check, small raise → min raise)` |
| 12 | 3 | `feat(game): showdown and split pot distribution` |
| 13 | 3 | `feat(game): player positions in DecisionContext` |
| 14 | 3 | `test(game): scripted strategies — blinds, min raise, side pots, chip conservation` |
| 15 | 3 | `feat(ui): ConsoleObserver with colored hand log` |
| 16 | 3 | `feat(ui): HumanStrategy keyboard input` |
| 17 | 3 | `feat(app): main menu, play vs bots, --seed and --no-color` |
| 18 | 4 | `feat(ui): hint command using Advisor` *(після злиття Advisor від Учасника 3)* |
| 19 | 4 | `feat(ui): rules and hand rankings screen` |

### Учасник 3 — аналітика та ШІ

| # | Тиждень | Коміт |
|---|---|---|
| 1 | 1 | `feat(analysis): Chen formula and preflop tiers` |
| 2 | 1 | `feat(analysis): hand codes and 13x13 preflop chart` |
| 3 | 1 | `feat(analysis): opening ranges by position` |
| 4 | 1 | `feat(analysis): pot odds, call EV and bluff break-even` |
| 5 | 2 | `feat(analysis): exact equity enumeration` |
| 6 | 2 | `feat(analysis): Monte Carlo equity and equity vs random hands` |
| 7 | 2 | `feat(analysis): outs counter with rule of 2 and 4` |
| 8 | 2 | `test(analysis): equity, odds, outs and chart` |
| 9 | 2 | `feat(ai): CallingStation and Random bots with factory` |
| 10 | 3 | `feat(analysis): Advisor preflop decisions` |
| 11 | 3 | `feat(analysis): Advisor postflop decisions (value bets, semi-bluffs)` |
| 12 | 3 | `feat(ai): TAG bot based on Advisor` |
| 13 | 3 | `feat(ai): LAG bot with bluffs and floats` |
| 14 | 3 | `test(ai): bot and advisor tests` |
| 15 | 3 | `feat(ui): equity calculator screen` |
| 16 | 4 | `feat(ui): situation analysis screen` |
| 17 | 4 | `feat(ui): preflop chart screen` |
| 18 | 4 | `feat(ui): bot tournament with bb/100, VPIP, PFR stats` |
| 19 | 4 | `docs: strategy and game theory notes` |

### Тиждень 5 — графічний інтерфейс

Спершу Учасник 1 додає raylib у CMake і `Render`, тоді двоє інших працюють паралельно.

| Учасник 1 | Учасник 2 | Учасник 3 |
|---|---|---|
| `build: fetch raylib with FetchContent` | `feat(gui): GameBridge — engine thread and event queue` | `feat(gui): analysis panel with hand strength and equity` |
| `feat(gui): fonts with Cyrillic and suit symbols` | `feat(gui): human decisions via condition variable` | `feat(gui): pot odds and EV in analysis panel` |
| `feat(gui): draw cards, card backs and chip stacks` | `feat(gui): table, seats, dealer button` | `feat(gui): outs list in analysis panel` |
| `feat(gui): buttons, panels, meters` | `feat(gui): deal and board animations` | `feat(gui): advisor recommendation and highlighted button` |
| `feat(eval): bestFiveCards for showdown highlight` | `feat(gui): action bar with raise slider and presets` | `feat(gui): preflop chart overlay with position ranges` |
| `ci: install X11 headers on Linux` | `feat(gui): start menu and game over screen` | `feat(gui): all-in runout equity like TV broadcast` |
| `docs: GUI section and screenshots in README` | `feat(gui): resizable window and HiDPI support` | `feat(gui): autoplay demo mode` |

### Тиждень 6 — сильніші боти

| Учасник 1 | Учасник 2 | Учасник 3 |
|---|---|---|
| `feat(game): sort hole cards high to low` | `feat(game): action history in DecisionContext` | `refactor(analysis): keep first advisor as BasicAdvisor` |
| `feat(gui): fanned hole cards with rotation` | `feat(game): opponent VPIP and aggression stats` | `feat(analysis): hand percentile ranking of 169 classes` |
| `test(game): hole card order and context history` | `feat(game): effective stack in DecisionContext` | `feat(analysis): equity vs opponent range` |
| | `feat(gui): stronger bot lineup` | `feat(analysis): EV-maximizing advisor with fold equity` |
| | | `feat(analysis): push/fold for short stacks` |
| | | `feat(gui): EV of every option in analysis panel` |
| | | `docs: advisor v2 and benchmark results` |

### Тиждень 7 — рівні складності та «Профі»

| Учасник 1 | Учасник 2 | Учасник 3 |
|---|---|---|
| `feat(game): per-player stats (PFR, fold to bet)` | `feat(game): action history in DecisionContext` | `feat(analysis): per-opponent ranges from hand history` |
| `feat(game): playerStats() like a poker tracker` | `feat(ai): difficulty levels and lineups` | `feat(analysis): ExpertAdvisor with personal fold model` |
| `feat(gui): stats tooltip on hover` | `feat(gui): difficulty selector in menu` | `feat(analysis): implied odds, overbets, blind steals` |
| `test(game): history and stats` | `feat(app): --level flag, difficulty in console` | `feat(ai): Expert, Expert-LAG and Novice bots` |
| `feat(gui): show amounts in big blinds` | `feat(gui): collapsible analysis panel` | `feat(gui): EV in big blinds in analysis panel` |
| `style(gui): smaller cards` | | `fix(analysis): expert folded to shove with good odds` |
| | | `fix(analysis): realistic 3-bet/4-bet sizing and commitment` |
| | | `feat(analysis): future-street value, no deep jams` |
| | | `test(ai): pot-odds and sizing regressions` |
| | | `test(ai): expert adapts to opponent fold frequency` |
| | | `docs: duplicate-poker benchmark results` |

### Тиждень 8 — інші карткові ігри (модуль `cards`)

| Учасник 1 | Учасник 2 | Учасник 3 |
|---|---|---|
| `feat(cards): preferans Card with Suit/Rank enums` | `feat(cards): GameRules and CardGame class` | `feat(cards): Monte Carlo estimate with std error` |
| `feat(cards): beat(x, y, trump)` | `feat(cards): presets — holdem, preferans, durak` | `feat(cards): four of a kind and straight flush checks` |
| `feat(cards): Array2D/Array3D containers` | `feat(cards): presets — blackjack, thousand, klondike` | `feat(cards): exact probabilities by enumeration` |
| `feat(cards): Decks with min rank, jokers, deck types` | `feat(app): cards_lab menu and --all report` | `feat(cards): poker tasks — random and fixed first hand` |
| `feat(cards): internal code suit*100+rank and conversion` | `feat(app): --seed and N options in cards_lab` | `feat(cards): preferans suit masks, 10-trick layouts` |
| `feat(cards): deal N×m×n with remainder` | `test(cards): game presets and weights` | `feat(cards): misère layouts, talon with 66 discards` |
| `feat(cards): fixed deal with excluded cards` | `docs: README section for cards_lab` | `feat(cards): trump split with hypergeometric check` |
| `test(cards): codes, deal shapes, fixed deal` | | `test(cards): preferans layouts and trump split` |
| | | `docs: TASKS.md with solutions and results` |

Разом ≈ 135 комітів + merge-коміти пул-реквестів + виправлення після рев'ю.

## Графік

| Тиждень | Мета | Результат |
|---|---|---|
| 1 | Каркас, карти, інтерфейси | `main` збирається, тести карт проходять |
| 2 | Оцінка рук, банки, еквіті | рушій грає роздачу між простими ботами |
| 3 | Порадник, боти, консоль | можна грати проти ботів |
| 4 | Аналітичні екрани, документація | консольна версія готова |
| 5 | Графічний інтерфейс | гра на столі з аналітикою |
| 6 | Сильніші боти (діапазони + EV) | середній рівень |
| 7 | Рівні складності, «Профі» | сильні боти трьох рівнів |
| 8 | Інші ігри: колоди, преферанс, ймовірності | `cards_lab --all`, готово до захисту |

## Чекліст перед захистом

- [ ] CI зелений на останньому коміті `main`
- [ ] `holdem_gui --seed 42` — демо-сценарій відрепетирувано (є і режим `--demo`)
- [ ] Кожен може пояснити свій модуль і відповісти на питання по коду
- [ ] README, STRATEGY.md і TASKS.md актуальні
- [ ] `cards_lab --all` відпрацьовує, цифри збігаються з TASKS.md (у межах похибки)
