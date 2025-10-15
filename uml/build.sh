#!/bin/bash

PLANTUML_LIMIT_SIZE=8192 plantuml -dots application_service_uml.puml
PLANTUML_LIMIT_SIZE=8192 plantuml -dots architecture_overview.puml
PLANTUML_LIMIT_SIZE=8192 plantuml -dots dependency_injection_diagram.puml
PLANTUML_LIMIT_SIZE=8192 plantuml -dots initialization_sequence.puml