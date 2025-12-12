NAME = fork-eater-language-server
VERSION = $(shell node -p "require('./package.json').version")

all: package

package: clean copy_libs compile
	@echo "📦 Packaging $(NAME)@$(VERSION)..."
	@npx vsce package --out $(NAME)-$(VERSION).vsix
	@echo "✅ Extension package created: $(NAME)-$(VERSION).vsix"

copy_libs:
	@echo "📂 Copying system libraries..."
	@mkdir -p server/libs
	@cp -r ../libs/* server/libs/

compile:
	@echo "🔨 Compiling client and server..."
	@npm run compile

clean:
	@echo "🧹 Cleaning up..."
	@rm -rf client/out server/out server/libs *.vsix
	@echo "✅ Cleanup complete"

.PHONY: all package compile clean
