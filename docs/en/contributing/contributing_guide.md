# Contributing to MindStudio Profiler

Thank you for considering contributing to MindStudio Profiler (msProf). We welcome all forms of contribution, including bug fixes, feature enhancements, documentation improvements, and even simple feedback. Whether you are an experienced developer or a first-time open-source contributor, your contributions are highly valued.

You can support this project in the following ways:

- Report issues through [Issues](https://gitcode.com/Ascend/msprof/issues).
- Propose or implement new features.
- Improve or expand the documentation.
- Review pull requests and help other contributors.
- Promote the project by sharing msProf in blog posts and on social media, or by starring the repository.

## Finding Issues to Contribute To

Want to start contributing? You can look for the following types of issues:

- [Good first issues](https://gitcode.com/Ascend/msprof/issues?q=is%3Aissue%20state%3Aopen%20label%3A%22good%20first%20issue%22)
- [Call for contribution](https://gitcode.com/Ascend/msprof/issues?q=is%3Aissue%20state%3Aopen%20label%3A%22help%20wanted%22)

You can also view the [Issues](https://gitcode.com/Ascend/msprof/issues) to learn about the project's development plans and roadmap.

## Contribution Workflow

### Environment Requirements

- For hardware requirements, see [Ascend Product Overview](https://www.hiascend.com/document/detail/en/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html).
- Install the open-source version of CANN in advance.
- Python 3.7.5 or later.
- CMake 3.14 or later.

### Development and Testing

1. Fork the source code repository to your personal repository and develop the code there.

   ```bash
   git clone https://gitcode.com/<your-username>/msprof.git
   cd msprof
   ```

2. Develop the code in your personal repository.

   Follow the [Code Guidelines](#code-guidelines) during development.

3. Test the code.

   See [Code Testing](#code-testing).

4. Build and test the code.

   Build the completed code into a wheel package for testing. For detailed instructions, see [MindStudio Profiler Installation Guide](../install_guide/msprof_install_guide.md).

5. Develop the documentation.

   If your changes involve adding, modifying, or removing features, provide the relevant documentation. For detailed requirements, see [Documentation Development](#documentation-development).

6. Submit a pull request.

   See [Pull Request Process](#pull-request-process).

### Code Guidelines

#### Python Code Guidelines

- Follow the PEP 8 coding conventions.
- Use four spaces for indentation.
- Use PascalCase for class names (such as `DataManager`).
- Use snake_case for function and variable names (such as `parse_data`).
- Add necessary type annotations and docstrings.

#### C++ Code Guidelines

- Follow the existing coding style of the project.
- Use four spaces for indentation.
- Use PascalCase for class names.
- Use camelCase for function names.
- Add necessary comments to explain complex logic.

### Code Testing

#### Running Tests

Before submitting code, ensure that all tests pass:

```bash
# Python unit tests
cd test/msprof_python/ut
python -m pytest

# C++ unit tests
cd test/msprof_cpp/analysis_ut
# Run tests using the applicable test framework.
```

#### Adding Tests

- Add corresponding unit tests for new features.
- Ensure that the tests cover the main logic branches.
- Ensure that test cases are readable and maintainable.
- Place test data in the appropriate locations under the `test/` directory.

#### Code Coverage

You can use the following scripts to generate code coverage reports:

```bash
# Python coverage
bash scripts/generate_coverage_py.sh

# C++ coverage
bash scripts/generate_coverage_cpp.sh
```

### Documentation Development

#### Documentation Paths

If your changes affect how users use the tool, update the relevant documentation:

- User guides: `docs/en/`
- API documentation: docstrings in the source code
- Code samples: `samples/`

#### Documentation Guidelines

- Use concise and clear phrasing.
- Provide complete code samples.
- Include necessary screenshots or diagrams.
- Ensure that links are valid.

### Pull Request Process

#### Pre-submission Checklist

Before submitting a pull request, ensure that:

- [ ] The code follows the project coding guidelines.
- [ ] Necessary test cases have been added.
- [ ] All tests pass.
- [ ] Related documentation has been updated.
- [ ] The commit message is clear and descriptive.
- [ ] The code has been self-reviewed.

#### Submission Process

1. **Create a branch**

   ```bash
   git checkout -b feature/<your-feature-name>
   ```

2. **Commit the changes**

   ```bash
   git add .
   git commit -m "feat: <your feature description>"
   ```

3. **Push to the remote repository**

   ```bash
   git push origin feature/<your-feature-name>
   ```

4. **Create a pull request**

   Create a pull request on GitCode and provide the following information:

   1. Clear title

      Follow the [Commit Message Guidelines](#commit-message-guidelines).

   2. Detailed description

      Include the changes made, reasons for the changes, testing details, and other relevant information.

   3. Associate the pull request with the relevant issue.

5. **Code review**

   1. After submitting a pull request, notify the relevant "owners" (Reviewers and Committers) to review the changes.
   2. Modify the code based on the review comments and resubmit the changes. Multiple iterations may be required during this process, so please respond promptly and maintain communication.

   The relevant "owners" will be identified during the pull request process. You can specify the relevant "owners" during the pull request process or contact the MindStudio team through the information provided in [README](../../../README_EN.md).

6. **Code merge**

   A pull request requires the following four labels to complete the merge:

   1. `ascend-cla/yes`: CLA check. First-time contributors must complete the CLA signing process. Subsequent submissions automatically receive this label.
   2. `ci-pipeline-passed`: CI pipeline. Comment `compile` on the pull request to trigger the CI pipeline. If the CI pipeline check fails, modify the code according to the prompts and resubmit it.
   3. `lgtm`: Provided by Reviewers. After the Reviewers approve the changes, comment `/lgtm` on the pull request to apply the `lgtm` label.
   4. `approved`: Provided by Committers. After the Committers approve the changes, comment `/approved` on the pull request to apply the `approved` label.

   Once your pull request has all four labels, it will be merged into the main branch.

#### Pull Request Best Practices

- Keep the size of pull requests moderate to facilitate review.
- Each pull request should address only one issue or implement one feature.
- Respond to review comments promptly.
- Keep your branch synchronized with the main branch and resolve conflicts promptly.

#### Commit Message Guidelines

Commit messages should clearly describe the changes and the reasons for them:

```bash
<type>: <subject>

<body>

<footer>
```

The `type` can be:

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation update
- `style`: Code formatting changes that do not affect functionality
- `refactor`: Code refactoring
- `test`: Test-related changes
- `chore`: Build process or auxiliary tool changes

Example:

```bash
feat: add memory usage analysis

- Implement the memory data collection module.
- Add a memory usage trend analysis algorithm.
- Update the relevant documentation.

Closes #123
```

## Community Guidelines

### Code of Conduct

We are committed to providing a friendly, safe, and inclusive environment for all participants. By participating in this project, you agree to:

- Respect different opinions and experiences.
- Accept constructive criticism.
- Focus on what is best for the community.
- Show empathy toward other community members.

### Communication Channels

- **Issues**: Report bugs, propose feature enhancements, and discuss technical issues.
- **Pull requests**: Review code and discuss specific implementations.
- **WeChat group**: Communicate and ask questions on a daily basis (see the MindStudio Team section in [README](../../../README.md)).

## License

By contributing code to this project, you agree to license your contributions under the project license. For details, see the [LICENSE](../../../LICENSE) file.

Documentation in the `docs` directory of the msProf tool is licensed under CC BY 4.0. For details, see [docs/LICENSE](../../LICENSE).

## Acknowledgments

Thank you for contributing to msProf. Your efforts make this project more powerful and user-friendly. We look forward to your participation.

---

If you have any questions or need assistance, feel free to submit an [issue](https://gitcode.com/Ascend/msprof/issues) or contact us through other community channels.
