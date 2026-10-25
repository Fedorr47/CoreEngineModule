# Gameplay component ownership

Gameplay component modules own state declarations and group them by the subdomain that gives the state meaning. They may depend on foundational value types such as math vectors, entity handles, and low-level AI state contracts, but must not import `GameplayWorld`, runtime systems, executors, or controllers.

`GameplayWorld` remains the sole owner of ECS storage, entity lifetime, deterministic entity collection, and generic component add, set, lookup, and removal APIs. Systems, actions, executors, and controllers own behavior and consume both the world API and the relevant component declarations.

Add a new component to its owning subdomain component module. Use `GameplayEntityComponents` only for genuinely cross-domain entity identity, transform, or scene/node binding state; do not add domain-specific declarations back to `GameplayWorld.cppm` or turn the foundational module into an umbrella component collection.
