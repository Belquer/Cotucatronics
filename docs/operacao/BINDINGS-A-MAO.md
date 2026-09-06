# Bindings à mão — Control Settings

> ⚠️ **NÃO É GERADO.** Foi feito à mão a partir do `CCS-POR-PAGINA.md`, que
> por sua vez também não é gerado — **derivado de um derivado**, e a
> distância à fonte multiplica-se. Vale o mesmo aviso que está lá.
>
> Feito de `CCS-POR-PAGINA.md`. Plano B do mapeamento: em Control
> Settings → perfil → **Add New Binding**, escolhe a ação e escreve o
> gatilho no campo **Trigger**.

> O caminho normal é o `gerar_perfil_loopy.py`, que emite tudo de uma vez.
> Esta folha existe para o dia em que a exportação não estiver disponível.


## CENTRAL  ·  9 bindings

| ✓ | param | Trigger CC | Trigger canal |
|---|---|---|---|
|  | `piezo_mix` | **20** | **ch4** |
|  | `dly_time` | **64** | **ch4** |
|  | `gate_thresh_a` | **23** | **ch5** |
|  | `gate_thresh_b` | **23** | **ch6** |
|  | `gate_release_a` | **25** | **ch5** |
|  | `gate_release_b` | **25** | **ch6** |
|  | `level` | **100** | **ch4** |

## MOD A  ·  16 bindings

| ✓ | param | Trigger CC | Trigger canal |
|---|---|---|---|
|  | `drv_on_a` | **30** | **ch5** |
|  | `drv_mix_a` | **31** | **ch5** |
|  | `drv_drive_a` | **34** | **ch5** |
|  | `drv_tone_a` | **35** | **ch5** |
|  | `drv_sujeira_a` | **36** | **ch5** |
|  | `phs_on_a` | **40** | **ch5** |
|  | `phs_mix_a` | **41** | **ch5** |
|  | `phs_rate_a` | **44** | **ch5** |
|  | `phs_depth_a` | **45** | **ch5** |
|  | `phs_fb_a` | **46** | **ch5** |
|  | `phs_stages_a` | **47** | **ch5** |
|  | `wrb_on_a` | **50** | **ch5** |
|  | `wrb_mix_a` | **51** | **ch5** |
|  | `wrb_rate_a` | **54** | **ch5** |
|  | `wrb_depth_a` | **55** | **ch5** |
|  | `wrb_irreg_a` | **56** | **ch5** |

## MOD B  ·  16 bindings

| ✓ | param | Trigger CC | Trigger canal |
|---|---|---|---|
|  | `drv_on_b` | **30** | **ch6** |
|  | `drv_mix_b` | **31** | **ch6** |
|  | `drv_drive_b` | **34** | **ch6** |
|  | `drv_tone_b` | **35** | **ch6** |
|  | `drv_sujeira_b` | **36** | **ch6** |
|  | `phs_on_b` | **40** | **ch6** |
|  | `phs_mix_b` | **41** | **ch6** |
|  | `phs_rate_b` | **44** | **ch6** |
|  | `phs_depth_b` | **45** | **ch6** |
|  | `phs_fb_b` | **46** | **ch6** |
|  | `phs_stages_b` | **47** | **ch6** |
|  | `wrb_on_b` | **50** | **ch6** |
|  | `wrb_mix_b` | **51** | **ch6** |
|  | `wrb_rate_b` | **54** | **ch6** |
|  | `wrb_depth_b` | **55** | **ch6** |
|  | `wrb_irreg_b` | **56** | **ch6** |

## TIME A  ·  14 bindings

| ✓ | param | Trigger CC | Trigger canal |
|---|---|---|---|
|  | `dly_on_a` | **60** | **ch5** |
|  | `dly_mix_a` | **61** | **ch5** |
|  | `frz_on_a` | **65** | **ch5** |
|  | `frz_vol_a` | **66** | **ch5** |
|  | `frz_size_a` | **67** | **ch5** |
|  | `rev_on_a` | **71** | **ch5** |
|  | `rev_mix_a` | **72** | **ch5** |
|  | `rev_size_a` | **73** | **ch5** |
|  | `rev_damp_a` | **74** | **ch5** |
|  | `shm_on_a` | **75** | **ch5** |
|  | `shm_mix_a` | **76** | **ch5** |
|  | `shm_feedback_a` | **77** | **ch5** |
|  | `shm_shift_a` | **78** | **ch5** |
|  | `shm_tone_a` | **79** | **ch5** |

## TIME B  ·  14 bindings

| ✓ | param | Trigger CC | Trigger canal |
|---|---|---|---|
|  | `dly_on_b` | **60** | **ch6** |
|  | `dly_mix_b` | **61** | **ch6** |
|  | `frz_on_b` | **65** | **ch6** |
|  | `frz_vol_b` | **66** | **ch6** |
|  | `frz_size_b` | **67** | **ch6** |
|  | `rev_on_b` | **71** | **ch6** |
|  | `rev_mix_b` | **72** | **ch6** |
|  | `rev_size_b` | **73** | **ch6** |
|  | `rev_damp_b` | **74** | **ch6** |
|  | `shm_on_b` | **75** | **ch6** |
|  | `shm_mix_b` | **76** | **ch6** |
|  | `shm_feedback_b` | **77** | **ch6** |
|  | `shm_shift_b` | **78** | **ch6** |
|  | `shm_tone_b` | **79** | **ch6** |

## RANDOM  ·  3 bindings

| ✓ | param | Trigger CC | Trigger canal |
|---|---|---|---|
|  | `buzios_on` | **90** | **ch4** |
|  | `buzios_thresh` | **91** | **ch4** |
|  | `buzios_cooldown` | **92** | **ch4** |

## TECH  ·  15 bindings

| ✓ | param | Trigger CC | Trigger canal |
|---|---|---|---|
|  | `tone_freq` | **93** | **ch4** |
|  | `tone_mix` | **94** | **ch4** |
|  | `tone_dest` | **95** | **ch4** |
|  | `test_tone_stage` | **96** | **ch4** |
|  | `exc_filter_l` | **105** | **ch5** |
|  | `exc_filter_r` | **105** | **ch6** |
|  | `gov_ceiling_l` | **110** | **ch5** |
|  | `gov_ceiling_r` | **110** | **ch6** |
|  | `gov_release_l` | **111** | **ch5** |
|  | `gov_release_r` | **111** | **ch6** |
|  | `shift_hz` | **112** | **ch4** |
|  | `out_l` | **113** | **ch4** |
|  | `out_r` | **114** | **ch4** |
|  | `route_a_l` | **97** | **ch5** |
|  | `route_b_l` | **97** | **ch6** |
|  | `route_a_r` | **98** | **ch5** |
|  | `route_b_r` | **98** | **ch6** |
|  | `bench_boost_l` | **115** | **ch5** |
|  | `bench_boost_r` | **115** | **ch6** |

---

**87 bindings no total.**
