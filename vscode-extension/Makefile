NAME = fork-eater-language-server
VERSION = $(shell node -p "require('./package.json').version")

all: package

package: clean index_libs compile
	@echo "📦 Packaging $(NAME)@$(VERSION)..."
	@npx vsce package --out $(NAME)-$(VERSION).vsix
	@echo "✅ Extension package created: $(NAME)-$(VERSION).vsix"

index_libs:
	@echo "📂 Generating system library index..."
	@node scripts/generate-libs-index.js

compile:
	@echo "🔨 Compiling client and server..."
	@npm run compile

clean:
	@echo "🧹 Cleaning up..."
	@rm -rf client/out server/out server/src/libs-index.json *.vsix
	@echo "✅ Cleanup complete"

.PHONY: all package compile clean
