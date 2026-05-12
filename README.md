Integrated Game README (Farm + Exploration)

This is an integrated embedded game that combines two core gameplay modes: a horizontal scrolling adventure combat exploration game and a farm simulation game. The two game modes are closely linked through shared data from the very beginning, creating a diverse and immersive gaming experience—players can switch freely between adventurous exploration and relaxing farm operations, with each mode supporting and complementing the other.
The exploration part draws inspiration from the classic Metal Slug, featuring horizontal combat adventure gameplay, while the farm part integrates casual and strategic elements such as smelting, resting, fishing and planting—all farm functions are available from the start, allowing players to balance combat challenges and peaceful farm management.
2. Core Gameplay
2.1 Exploration Game
The exploration gameplay focuses on combat, survival and scene exploration, and is closely connected with the farm mode through shared data from the initial stage:
2.1.1 Combat System
Three distinct enemy types are available: Melee Enemy (fast movement, close-range attack), Laser Ranged Enemy (long-range laser attack, obstacle avoidance), and Heavy Enemy (high health, high armor, area damage). Players can use four weapons to fight: Machine Gun, Submachine Gun, Laser Gun, and Light Sword, each with unique damage, fire rate and ammo consumption rules.
2.1.2 Survival Mechanics
Players have three core attributes: HP, Armor, and Hunger. Armor absorbs damage before HP is consumed; Hunger depletes over time, affecting player movement speed if it is too low. Players can obtain items through enemy drops or supply points to maintain survival, and these items can be used interchangeably with the farm mode.
2.1.3 Exploration & Special Mechanics
The game has four maps, and players can explore different scenes through portals. Altars in the map support save and character enhancement, while supply points provide timely recovery of HP, armor and ammo. The HUD interface clearly displays key information: HP, Hunger and Armor at the top, and equipped weapons, inventory and map at the bottom.
2.2 Farm Game
The farm mode is designed as a casual and strategic supplement to the exploration mode, with four core operations that are all available from the start and closely linked to the exploration mode through shared inventory data:
2.2.1 Smelting
Players can smelt materials obtained from exploration (e.g., metal fragments dropped by enemies) into armor. Weapon-making does not require smelted metal materials, and the smelted armor can be directly used in the exploration mode to absorb damage, or stored in the shared backpack for later use.
2.2.2 Resting
Resting in the farm can quickly recover the player’s HP and eliminate hunger, avoiding the need to consume a large number of items during exploration. Resting time is adjustable, and it can be carried out while processing other farm operations (e.g., waiting for crops to mature).
2.2.3 Fishing
Fishing is an important way to obtain food in the farm mode. The fish caught can be directly consumed to restore hunger (used in both modes) or stored in the shared backpack, providing a stable food supply for long-term exploration.
2.2.4 Planting
Players can plant seeds (obtained from exploration or farm rewards) in the farm. After the crops mature, they can be harvested as food (to restore hunger) or processed into other materials. The seeds and harvested crops are stored in the shared backpack, realizing resource circulation between the two modes.
2.3 Shared Data Between Two Modes
The core connection between the two game modes is the shared backpack, which synchronizes data in real time, ensuring that resources obtained in one mode can be freely used in the other. The shared backpack includes the following key items:
- Armor: Obtained through exploration (enemy drops, supply points) or smelted in the farm, used to absorb damage in the exploration mode.
- Weapon-making Materials: Obtained directly from exploration, no smelting required; used to maintain or enhance weapons in the exploration mode.
- Seeds: Obtained through exploration or farm rewards, used for planting in the farm mode to produce food or other resources.
- Food: Obtained through exploration (enemy drops, supply points), fishing or crop harvesting in the farm, used to restore hunger in both modes.
2.4 Audio-Visual Experience
The game integrates a complete sound system, including background music and in-game sound effects (weapon fire, enemy attacks, item pickups, etc.), and adds farm-specific sound effects (fishing, planting, smelting) to enhance immersion. Short CG clips are added for key moments (activating altars, farm operation prompts) to connect the two game modes smoothly. The background of the exploration mode adopts real painting + texture mapping method, while the farm background uses a warm, cartoon-style design, distinguishing the two modes while maintaining visual consistency.
3. Development Process
The game development integrates both exploration and farm modes from the beginning, with the two modes coexisting and interacting from the initial development stage. The core development focus includes:
- Completed the development of both exploration and farm modes simultaneously, including the full set of farm functions (smelting, resting, fishing and planting) that are available from the start, as well as the core exploration gameplay (system setup, combat, UI, sound, background optimization, etc.).
- Developed a shared data system, focusing on the real-time synchronization of the shared backpack, ensuring that items (armor, weapon-making materials, seeds, food) can be freely used and updated between the two modes.
- Optimized the HUD interface to support free switching between exploration and farm modes, and added farm-specific status display (e.g., crop maturity time, smelting progress).
- Completed the integration and testing of the two modes, fixed data synchronization bugs, and adjusted the balance between the two modes to ensure a smooth gaming experience.
4. Key Technical Points
- Modular Design: The project retains the original include/ (header files/interfaces) and src/ (implementation files) structure, and adds a farm module (farm.cpp/h) to ensure code organization and scalability.
- Background Optimization: The exploration mode retains the WebP format map (each 80+ KB) to avoid game crashes; the farm mode uses lightweight background resources to ensure smooth switching between modes.
- Shared Data Synchronization: Realized real-time synchronization of the shared backpack, ensuring that items obtained in either mode are updated immediately in the backpack, and can be used in the other mode without delay.
- Dual-mode Integration: Realized seamless switching between exploration and farm modes, optimized the system to avoid lag during mode switching, and ensured stable operation of all modules (combat, farm, UI, sound, CG, shared data).
5. Environment Requirements
Embedded system environment, supporting C++ compilation, and relevant dependencies for image (WebP format) and sound rendering; additional support for real-time data synchronization between dual modes is required.
6. Notes
- All map files of the exploration mode are in WebP format to ensure game running efficiency; farm background resources are lightweight to avoid affecting mode switching speed.
- The console module provides debugging support for system optimization, bug fixing, and shared data synchronization testing.
- CG clips are used to connect key moments of the two modes, enhancing the narrative and gaming experience.
- The shared backpack data is synchronized in real time; it is recommended not to modify the item data manually to avoid data confusion between the two modes.
- Seeds and smelted materials in the farm mode are closely linked to the exploration mode; reasonable allocation of resources between the two modes can improve the overall gaming efficiency.
