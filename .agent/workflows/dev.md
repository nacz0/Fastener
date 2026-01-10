---
description: Standardowy workflow programowania w Fastener
---

Ten workflow zapewnia, że AI zawsze korzysta z najświeższej dokumentacji technicznej przed wygenerowaniem kodu.

1. Przeczytaj plik `.agent/CONTEXT.md` dla szybkiego kontekstu (~30 sekund).
2. Jeśli potrzebujesz sygnatur API, sprawdź `docs/SIGNATURES.json`.
3. Dla pełnego kontekstu przeczytaj `docs/AI_CONTEXT.md`.
4. Zapoznaj się z plikiem `include/fastener/fastener.h`, aby potwierdzić dostępność nagłówków.
5. Wykonaj zadanie programistyczne zlecone przez użytkownika, dbając o spójność z paradygmatem Immediate-Mode.

