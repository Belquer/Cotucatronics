# Bindings à mão — Control Settings

> **Gerado de `CCS-POR-PAGINA.md`.** Não usar MIDI Learn: em Control
> Settings → perfil → **Add New Binding**, escolhe a ação e escreve o
> gatilho no campo **Trigger**. Sem inferência de gesto, sem janelas de
> 250/500 ms, sem double tap.

**Antes de começar:** Control Settings → MIDI Devices → WIDI → desligar
**Feedback Enabled**. O retorno automático sai no canal do *binding* —
os nossos canais de escuta — e colide com o envio explícito.

O que se escreve no Trigger é o canal de **escuta**: é por onde a placa
manda para o Loopy, e é o que faz o widget seguir o estado da placa.


## CENTRAL  ·  8 bindings

| ✓ | param | tipo | Trigger CC | Trigger canal |
|---|---|---|---|---|
|  | `piezo_mix` | contínuo | **20** | **ch4** |
|  | `in_gain_a` | contínuo | **21** | **ch5** |
|  | `in_gain_b` | contínuo | **21** | **ch6** |
|  | `gate_thresh_a` | contínuo | **23** | **ch5** |
|  | `gate_thresh_b` | contínuo | **23** | **ch6** |
|  | `gate_release_a` | contínuo | **25** | **ch5** |
|  | `gate_release_b` | contínuo | **25** | **ch6** |
|  | `level` | contínuo | **100** | **ch4** |

## MOD A  ·  16 bindings

| ✓ | param | tipo | Trigger CC | Trigger canal |
|---|---|---|---|---|
|  | `drv_on_a` | bool | **30** | **ch5** |
|  | `drv_mix_a` | contínuo | **31** | **ch5** |
|  | `drv_drive_a` | contínuo | **34** | **ch5** |
|  | `drv_tone_a` | contínuo | **35** | **ch5** |
|  | `drv_sujeira_a` | contínuo | **36** | **ch5** |
|  | `phs_on_a` | bool | **40** | **ch5** |
|  | `phs_mix_a` | contínuo | **41** | **ch5** |
|  | `phs_rate_a` | contínuo | **44** | **ch5** |
|  | `phs_depth_a` | contínuo | **45** | **ch5** |
|  | `phs_fb_a` | contínuo | **46** | **ch5** |
|  | `phs_stages_a` | contínuo | **47** | **ch5** |
|  | `wrb_on_a` | bool | **50** | **ch5** |
|  | `wrb_mix_a` | contínuo | **51** | **ch5** |
|  | `wrb_rate_a` | contínuo | **54** | **ch5** |
|  | `wrb_depth_a` | contínuo | **55** | **ch5** |
|  | `wrb_irreg_a` | contínuo | **56** | **ch5** |

## MOD B  ·  16 bindings

| ✓ | param | tipo | Trigger CC | Trigger canal |
|---|---|---|---|---|
|  | `drv_on_b` | bool | **30** | **ch6** |
|  | `drv_mix_b` | contínuo | **31** | **ch6** |
|  | `drv_drive_b` | contínuo | **34** | **ch6** |
|  | `drv_tone_b` | contínuo | **35** | **ch6** |
|  | `drv_sujeira_b` | contínuo | **36** | **ch6** |
|  | `phs_on_b` | bool | **40** | **ch6** |
|  | `phs_mix_b` | contínuo | **41** | **ch6** |
|  | `phs_rate_b` | contínuo | **44** | **ch6** |
|  | `phs_depth_b` | contínuo | **45** | **ch6** |
|  | `phs_fb_b` | contínuo | **46** | **ch6** |
|  | `phs_stages_b` | contínuo | **47** | **ch6** |
|  | `wrb_on_b` | bool | **50** | **ch6** |
|  | `wrb_mix_b` | contínuo | **51** | **ch6** |
|  | `wrb_rate_b` | contínuo | **54** | **ch6** |
|  | `wrb_depth_b` | contínuo | **55** | **ch6** |
|  | `wrb_irreg_b` | contínuo | **56** | **ch6** |

## TIME A  ·  14 bindings

| ✓ | param | tipo | Trigger CC | Trigger canal |
|---|---|---|---|---|
|  | `dly_on_a` | bool | **60** | **ch5** |
|  | `dly_mix_a` | contínuo | **61** | **ch5** |
|  | `frz_on_a` | bool | **65** | **ch5** |
|  | `frz_vol_a` | contínuo | **66** | **ch5** |
|  | `frz_size_a` | contínuo | **67** | **ch5** |
|  | `rev_on_a` | bool | **71** | **ch5** |
|  | `rev_mix_a` | contínuo | **72** | **ch5** |
|  | `rev_size_a` | contínuo | **73** | **ch5** |
|  | `rev_damp_a` | contínuo | **74** | **ch5** |
|  | `shm_on_a` | bool | **75** | **ch5** |
|  | `shm_mix_a` | contínuo | **76** | **ch5** |
|  | `shm_feedback_a` | contínuo | **77** | **ch5** |
|  | `shm_shift_a` | contínuo | **78** | **ch5** |
|  | `shm_tone_a` | contínuo | **79** | **ch5** |

## TIME B  ·  14 bindings

| ✓ | param | tipo | Trigger CC | Trigger canal |
|---|---|---|---|---|
|  | `dly_on_b` | bool | **60** | **ch6** |
|  | `dly_mix_b` | contínuo | **61** | **ch6** |
|  | `frz_on_b` | bool | **65** | **ch6** |
|  | `frz_vol_b` | contínuo | **66** | **ch6** |
|  | `frz_size_b` | contínuo | **67** | **ch6** |
|  | `rev_on_b` | bool | **71** | **ch6** |
|  | `rev_mix_b` | contínuo | **72** | **ch6** |
|  | `rev_size_b` | contínuo | **73** | **ch6** |
|  | `rev_damp_b` | contínuo | **74** | **ch6** |
|  | `shm_on_b` | bool | **75** | **ch6** |
|  | `shm_mix_b` | contínuo | **76** | **ch6** |
|  | `shm_feedback_b` | contínuo | **77** | **ch6** |
|  | `shm_shift_b` | contínuo | **78** | **ch6** |
|  | `shm_tone_b` | contínuo | **79** | **ch6** |

## RANDOM  ·  3 bindings

| ✓ | param | tipo | Trigger CC | Trigger canal |
|---|---|---|---|---|
|  | `buzios_on` | bool | **90** | **ch4** |
|  | `buzios_thresh` | contínuo | **91** | **ch4** |
|  | `buzios_cooldown` | contínuo | **92** | **ch4** |

## TECH  ·  14 bindings

| ✓ | param | tipo | Trigger CC | Trigger canal |
|---|---|---|---|---|
|  | `tone_freq` | contínuo | **93** | **ch4** |
|  | `tone_mix` | contínuo | **94** | **ch4** |
|  | `tone_dest` | contínuo | **95** | **ch4** |
|  | `exc_on_a` | bool | **105** | **ch5** |
|  | `exc_on_b` | bool | **105** | **ch6** |
|  | `gov_ceiling_a` | contínuo | **110** | **ch5** |
|  | `gov_ceiling_b` | contínuo | **110** | **ch6** |
|  | `gov_release_a` | contínuo | **111** | **ch5** |
|  | `gov_release_b` | contínuo | **111** | **ch6** |
|  | `shift_hz` | contínuo | **112** | **ch4** |
|  | `route_exciter` | bool | **113** | **ch4** |
|  | `route_jack` | bool | **114** | **ch4** |
|  | `bench_boost_a` | contínuo | **115** | **ch5** |
|  | `bench_boost_b` | contínuo | **115** | **ch6** |

---

**85 bindings no total.**

Se o painel de Trigger oferecer *Hold* ou *Double Tap*, o alvo escolhido
é uma ação **impulse** — errado para parâmetro contínuo. Contínuo mostra
*Absolute/Relative*; usa **Absolute**.

⚠️ Falta o `dly_time` (cc64, ch1/ch4): é compartilhado e não aparece em
página nenhuma do documento de origem. Acrescentar à CENTRAL.
