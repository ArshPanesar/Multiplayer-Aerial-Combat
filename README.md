# Multiplayer Aerial Combat

This is a prototype for a networked aerial combat game, built in **Unreal Engine 5**, which uses custom networked vehicle physics, combat mechanics, 
and a procedurally generated city.

## Links

### Gameplay Video

You can click on the Image to watch the Gameplay video. 

[![Watch Youtube Video Here](https://img.youtube.com/vi/Z01IaK6mlR8/0.jpg)](https://www.youtube.com/watch?v=Z01IaK6mlR8)

### Download Development Build on Itch.io

#### [Itch](https://arsh-panesar.itch.io/aerial-combat-multiplayer)


## Key Features

- **Custom Flying Vehicle Physics**
  - Fully functional hover and flight dynamics.
  - Custom Client-side prediction and server reconciliation for smooth multiplayer control.
  - Mostly Written in C++, with the exception of UI Handling and Respawn Requests done in Blueprints. 

- **Networked Combat**
  - Projectile-based shooting system with network prediction via Gameplay Ability System (GAS).
  - Based on Sam Reitich's [Article on Projectile Prediction](https://sreitich.github.io/projectile-prediction-1/)).
  - Written in C++.

- **Procedural City Generation**
  - A City with an Octagonal Layout, built using Unreal’s PCG Graph.
  - Built using PCG Graph Nodes.

- **UI and VFX**
  - Engine trails, hovering effects, and impact visuals done using Niagara and Material Graph.
  - UI Built using Widget Blueprints.


