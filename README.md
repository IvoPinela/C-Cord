# 💬 C-Cord: Sistema de Comunicação Segura em Tempo Real

O **C-Cord** é uma plataforma de comunicação inspirada no Discord, desenvolvida em linguagem **C**. O projeto foca-se na evolução de um servidor sequencial básico para um sistema de alto desempenho com suporte a múltiplos canais e segurança avançada.

---

## 🚀 Visão Geral

O sistema baseia-se numa arquitetura Cliente-Servidor, progredindo através de quatro fases principais:

1.  **Fase 1 e 2:** Implementação de um servidor sequencial utilizando *sockets* bloqueantes para autenticação e gestão de mensagens simples.
2.  **Fase 3:** Upgrade para tempo real utilizando multiplexagem (`select()` ou `poll()`), permitindo transmissões instantâneas (*broadcast*) e múltiplos canais simultâneos.
3.  **Fase 4:** Implementação de segurança robusta para garantir Confidencialidade, Integridade e Autenticidade (CIA).

---

## 👥 Estrutura da Equipa (Etapa 1)

| Função | Nome | Responsabilidades |
| :--- | :--- | :--- |
| **Team Manager** | Carlos Martins | Coordenação, planeamento (Gantt) e apresentações. |
| **Account Manager** | Miguel Rodrigues | Requisitos funcionais/não-funcionais e mockups (Figma). |
| **Software Manager** | Ricardo Pereira | Arquitetura do sistema e gestão de ferramentas (GitHub). |
| **Risk & Testing Manager** | Diego França | Plano de mitigação de riscos e protocolos de testes. |
| **Quality Manager** | Rui Pina | Estado da arte e garantia de qualidade documental. |
| **Dev Team** | David Bunga, Jucimar Cabral, Ivo Pinela | Desenvolvimento técnico das funcionalidades (F1 a F15). |

---

## 📅 Datas Críticas (2026)

* **14/05:** Entrega da constituição dos grupos.
* **16/05:** Entrega e Defesa da Etapa 1 (Funcionalidades F1-F4).
* **29/05:** Entrega e Defesa da Etapa 2 (Funcionalidades F5-F8).
* **12/06:** Entrega da Etapa 3 (Funcionalidades F9-F10).
* **05/07:** Entrega Final (Funcionalidades F11-F15).

---


---
> **Aviso Académico:** Este projeto é um exercício de aprendizagem de algoritmos criptográficos. Em ambientes de produção, devem ser utilizadas bibliotecas auditadas como OpenSSL ou libsodium.
